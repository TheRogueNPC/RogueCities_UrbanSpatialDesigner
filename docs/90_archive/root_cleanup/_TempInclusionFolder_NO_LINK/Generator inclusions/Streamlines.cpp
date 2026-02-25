#include "Streamlines.h"

#include <cmath>
#include <limits>
#include <stack>
#include <algorithm>

#if RCG_USE_GEOS
#include <geos_c.h>
#endif

namespace
{
    constexpr double PI = 3.14159265358979323846;

    double perpendicularDot(const CityModel::Vec2 &a, const CityModel::Vec2 &b)
    {
        return a.x * b.y - a.y * b.x;
    }

    double pointLineDistance(const CityModel::Vec2 &p, const CityModel::Vec2 &a, const CityModel::Vec2 &b)
    {
        const double area = std::abs((b.x - a.x) * (a.y - p.y) - (a.x - p.x) * (b.y - a.y));
        const double len = std::sqrt((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y));
        return len == 0 ? 0 : area / len;
    }

#if RCG_USE_GEOS
    std::vector<CityModel::Vec2> geos_simplify_line(const std::vector<CityModel::Vec2> &line, double tolerance)
    {
        if (line.size() < 2)
            return line;

        GEOSContextHandle_t ctx = GEOS_init_r();
        if (!ctx)
            return line;

        GEOSCoordSequence *seq = GEOSCoordSeq_create_r(ctx, static_cast<unsigned int>(line.size()), 2);
        if (!seq)
        {
            GEOS_finish_r(ctx);
            return line;
        }

        for (std::size_t i = 0; i < line.size(); ++i)
        {
            GEOSCoordSeq_setX_r(ctx, seq, static_cast<unsigned int>(i), line[i].x);
            GEOSCoordSeq_setY_r(ctx, seq, static_cast<unsigned int>(i), line[i].y);
        }

        GEOSGeometry *geom = GEOSGeom_createLineString_r(ctx, seq);
        if (!geom)
        {
            GEOSCoordSeq_destroy_r(ctx, seq);
            GEOS_finish_r(ctx);
            return line;
        }

        GEOSGeometry *simplified = GEOSTopologyPreserveSimplify_r(ctx, geom, tolerance);
        if (!simplified)
        {
            GEOSGeom_destroy_r(ctx, geom);
            GEOS_finish_r(ctx);
            return line;
        }

        const GEOSCoordSequence *outSeq = GEOSGeom_getCoordSeq_r(ctx, simplified);
        unsigned int outSize = 0;
        std::vector<CityModel::Vec2> out;
        if (outSeq && GEOSCoordSeq_getSize_r(ctx, outSeq, &outSize) && outSize >= 2)
        {
            out.reserve(outSize);
            for (unsigned int i = 0; i < outSize; ++i)
            {
                double x = 0.0;
                double y = 0.0;
                GEOSCoordSeq_getX_r(ctx, outSeq, i, &x);
                GEOSCoordSeq_getY_r(ctx, outSeq, i, &y);
                out.push_back({x, y});
            }
        }

        GEOSGeom_destroy_r(ctx, simplified);
        GEOSGeom_destroy_r(ctx, geom);
        GEOS_finish_r(ctx);

        if (out.size() >= 2)
            return out;
        return line;
    }
#endif
} // namespace

struct StreamlineIntegration
{
    CityModel::Vec2 seed;
    CityModel::Vec2 originalDir;
    std::vector<CityModel::Vec2> streamline;
    CityModel::Vec2 previousDirection;
    CityModel::Vec2 previousPoint;
    bool valid{false};
};

StreamlineGenerator::StreamlineGenerator(const FieldIntegrator &integ,
                                         const CityModel::Vec2 &originIn,
                                         const CityModel::Vec2 &worldDims,
                                         const StreamlineParams &p,
                                         CityModel::RNG rngIn)
    : integrator(integ),
      origin(originIn),
      worldDimensions(worldDims),
      params(p),
      majorGrid(worldDims, originIn, p.dsep),
      minorGrid(worldDims, originIn, p.dsep),
      rng(std::move(rngIn))
{
    // Enforce constraints similar to TS
    params.dtest = std::min(params.dtest, params.dsep);
    dcollideselfSq = std::pow(params.dcirclejoin / 2.0, 2);
    nStreamlineStep = static_cast<int>(std::floor(params.dcirclejoin / params.dstep));
    nStreamlineLookBack = 2 * nStreamlineStep;

    paramsSq = params;
    paramsSq.dsep *= paramsSq.dsep;
    paramsSq.dtest *= paramsSq.dtest;
    paramsSq.dstep *= paramsSq.dstep;
    paramsSq.dcirclejoin *= paramsSq.dcirclejoin;
}

void StreamlineGenerator::clearStreamlines()
{
    allStreamlines.clear();
    allStreamlinesSimple.clear();
    streamlinesMajor.clear();
    streamlinesMinor.clear();
}

CityModel::Vec2 StreamlineGenerator::samplePoint()
{
    return CityModel::Vec2{
        rng.uniform(worldDimensions.x) + origin.x,
        rng.uniform(worldDimensions.y) + origin.y};
}

CityModel::Vec2 StreamlineGenerator::getSeed(bool major)
{
    CityModel::Vec2 seed = samplePoint();
    int tries = 0;
    while (!isValidSample(major, seed, paramsSq.dsep))
    {
        if (tries++ >= params.seedTries)
        {
            return CityModel::Vec2{-1e9, -1e9}; // invalid sentinel
        }
        seed = samplePoint();
    }
    return seed;
}

bool StreamlineGenerator::isValidSample(bool major, const CityModel::Vec2 &point, double dSq, bool bothGrids) const
{
    bool gridValid = grid(major).isValidSample(point, dSq);
    if (bothGrids)
    {
        gridValid = gridValid && grid(!major).isValidSample(point, dSq);
    }
    const double influence = integrator.influenceAt(point);
    return integrator.onLand(point) && gridValid && influence > 0.05;
}

bool StreamlineGenerator::pointInBounds(const CityModel::Vec2 &v) const
{
    return (v.x >= origin.x &&
            v.y >= origin.y &&
            v.x < worldDimensions.x + origin.x &&
            v.y < worldDimensions.y + origin.y);
}

bool StreamlineGenerator::streamlineTurned(const CityModel::Vec2 &seed,
                                           const CityModel::Vec2 &originalDir,
                                           const CityModel::Vec2 &point,
                                           const CityModel::Vec2 &direction) const
{
    if (originalDir.dot(direction) < 0)
    {
        const CityModel::Vec2 perp{originalDir.y, -originalDir.x};
        const bool isLeft = (point.clone().sub(seed)).dot(perp) < 0;
        const bool directionUp = direction.dot(perp) > 0;
        return isLeft == directionUp;
    }
    return false;
}

void StreamlineGenerator::streamlineIntegrationStep(StreamlineIntegration &paramsStruct, bool major, bool collideBoth)
{
    if (!paramsStruct.valid)
        return;

    paramsStruct.streamline.push_back(paramsStruct.previousPoint);
    CityModel::Vec2 nextDirection = integrator.integrate(paramsStruct.previousPoint, major);
    if (nextDirection.lengthSquared() < 0.01)
    {
        paramsStruct.valid = false;
        return;
    }
    if (nextDirection.dot(paramsStruct.previousDirection) < 0)
    {
        nextDirection.multiply(-1.0);
    }
    CityModel::Vec2 nextPoint = paramsStruct.previousPoint.clone().add(nextDirection);

    if (pointInBounds(nextPoint) && isValidSample(major, nextPoint, paramsSq.dtest, collideBoth) && !streamlineTurned(paramsStruct.seed, paramsStruct.originalDir, nextPoint, nextDirection))
    {
        paramsStruct.previousPoint = nextPoint;
        paramsStruct.previousDirection = nextDirection;
    }
    else
    {
        paramsStruct.streamline.push_back(nextPoint);
        paramsStruct.valid = false;
    }
}

std::vector<CityModel::Vec2> StreamlineGenerator::integrateStreamline(const CityModel::Vec2 &seed, bool major)
{
    if (seed.x < -1e8)
        return {}; // invalid seed
    int count = 0;
    bool pointsEscaped = false;
    const bool collideBoth = rng.uniform() < params.collideEarly;

    CityModel::Vec2 d = integrator.integrate(seed, major);
    StreamlineIntegration forward{seed, d, {seed}, d, seed.clone().add(d), true};
    forward.valid = pointInBounds(forward.previousPoint);

    CityModel::Vec2 negD = d.clone().multiply(-1.0);
    StreamlineIntegration backward{seed, negD, {}, negD, seed.clone().add(negD), true};
    backward.valid = pointInBounds(backward.previousPoint);

    while (count < params.pathIterations && (forward.valid || backward.valid))
    {
        streamlineIntegrationStep(forward, major, collideBoth);
        streamlineIntegrationStep(backward, major, collideBoth);

        const double distSq = forward.previousPoint.distanceToSquared(backward.previousPoint);
        if (!pointsEscaped && distSq > paramsSq.dcirclejoin)
        {
            pointsEscaped = true;
        }
        if (pointsEscaped && distSq <= paramsSq.dcirclejoin)
        {
            forward.streamline.push_back(forward.previousPoint);
            forward.streamline.push_back(backward.previousPoint);
            backward.streamline.push_back(backward.previousPoint);
            break;
        }
        ++count;
    }

    std::reverse(backward.streamline.begin(), backward.streamline.end());
    backward.streamline.insert(backward.streamline.end(), forward.streamline.begin(), forward.streamline.end());
    return backward.streamline;
}

std::vector<CityModel::Vec2> StreamlineGenerator::simplifyStreamline(const std::vector<CityModel::Vec2> &s) const
{
    if (s.size() < 3)
        return s;

    const double tolerance = params.simplifyTolerance;

#if RCG_USE_GEOS
    if (tolerance > 0.0)
        return geos_simplify_line(s, tolerance);
#endif

    std::vector<char> keep(s.size(), 0);
    keep.front() = keep.back() = 1;

    std::stack<std::pair<std::size_t, std::size_t>> st;
    st.push({0, s.size() - 1});
    while (!st.empty())
    {
        auto [start, end] = st.top();
        st.pop();
        double maxDist = 0.0;
        std::size_t index = start;
        for (std::size_t i = start + 1; i < end; ++i)
        {
            double d = pointLineDistance(s[i], s[start], s[end]);
            if (d > maxDist)
            {
                maxDist = d;
                index = i;
            }
        }
        if (maxDist > tolerance)
        {
            keep[index] = 1;
            st.push({start, index});
            st.push({index, end});
        }
    }

    std::vector<CityModel::Vec2> out;
    for (std::size_t i = 0; i < s.size(); ++i)
        if (keep[i])
            out.push_back(s[i]);
    return out;
}

std::vector<CityModel::Vec2> StreamlineGenerator::complexifyStreamline(const std::vector<CityModel::Vec2> &s) const
{
    std::vector<CityModel::Vec2> out;
    if (s.empty())
        return out;
    for (std::size_t i = 0; i + 1 < s.size(); ++i)
    {
        auto seg = complexifySegment(s[i], s[i + 1]);
        if (!out.empty())
            seg.erase(seg.begin()); // avoid duplicate
        out.insert(out.end(), seg.begin(), seg.end());
    }
    return out;
}

std::vector<CityModel::Vec2> StreamlineGenerator::complexifySegment(const CityModel::Vec2 &v1, const CityModel::Vec2 &v2) const
{
    if (v1.distanceToSquared(v2) <= paramsSq.dstep)
    {
        return {v1, v2};
    }
    CityModel::Vec2 d = v2.clone().sub(v1);
    CityModel::Vec2 mid = v1.clone().add(CityModel::Vec2{d.x * 0.5, d.y * 0.5});
    auto left = complexifySegment(v1, mid);
    auto right = complexifySegment(mid, v2);
    left.insert(left.end(), right.begin() + 1, right.end());
    return left;
}

std::vector<CityModel::Vec2> StreamlineGenerator::pointsBetween(const CityModel::Vec2 &v1, const CityModel::Vec2 &v2, double dstep) const
{
    double d = v1.distanceTo(v2);
    int nPoints = static_cast<int>(std::floor(d / dstep));
    if (nPoints == 0)
        return {};
    CityModel::Vec2 stepVec = v2.clone().sub(v1);
    std::vector<CityModel::Vec2> out;
    for (int i = 1; i <= nPoints; ++i)
    {
        CityModel::Vec2 next = v1.clone().add(CityModel::Vec2{stepVec.x * (static_cast<double>(i) / nPoints),
                                                              stepVec.y * (static_cast<double>(i) / nPoints)});
        if (integrator.integrate(next, true).lengthSquared() > 0.001)
        {
            out.push_back(next);
        }
        else
        {
            break;
        }
    }
    return out;
}
