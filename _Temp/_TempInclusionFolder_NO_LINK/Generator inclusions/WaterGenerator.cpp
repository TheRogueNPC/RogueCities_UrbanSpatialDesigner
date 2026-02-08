#include "WaterGenerator.h"

#include "PolygonUtil.h"
#include "DebugLog.hpp"
#include <algorithm>

namespace WaterGenerator
{

    WaterGenerator::WaterGenerator(const FieldIntegrator &integrator,
                                   const CityModel::Vec2 &origin,
                                   const CityModel::Vec2 &worldDimensions,
                                   const WaterParams &paramsIn,
                                   TensorField::TensorField &field,
                                   CityModel::RNG rng)
        : StreamlineGenerator(integrator, origin, worldDimensions, paramsIn, std::move(rng)),
          paramsWater(paramsIn),
          tensorField(field) {}

    void WaterGenerator::createCoast()
    {
        RCG::DebugLog::printf("[WaterGen] createCoast start\n");
        std::vector<CityModel::Vec2> coast;
        CityModel::Vec2 seed;
        bool major = true;

        if (paramsWater.coastNoise.noiseEnabled)
        {
            tensorField.enableGlobalNoise(paramsWater.coastNoise.noiseAngle, paramsWater.coastNoise.noiseSize);
        }
        for (int i = 0; i < TRIES; ++i)
        {
            major = rng.uniform() < 0.5;
            seed = getSeed(major);
            coast = extendStreamline(integrateStreamline(seed, major));
            if (reachesEdges(coast))
                break;
        }
        tensorField.disableGlobalNoise();

        coastlineLine = coast;
        coastlineMajor = major;

        auto road = simplifyStreamline(coast);
        seaPoly = getSeaPolygon(road);
        allStreamlinesSimple.push_back(road);
        tensorField.sea = CityModel::Polyline{seaPoly};

        auto complex = complexifyStreamline(road);
        grid(major).addPolyline(complex);
        streamlines(major).push_back(complex);
        allStreamlines.push_back(complex);
        RCG::DebugLog::printf("[WaterGen] createCoast done points=%zu\n", coastlineLine.size());
    }

    void WaterGenerator::createRiver()
    {
        RCG::DebugLog::printf("[WaterGen] createRiver start\n");
        std::vector<CityModel::Vec2> river;
        CityModel::Vec2 seed;

        // Temporarily ignore sea for edge checks
        auto oldSea = tensorField.sea;
        tensorField.sea.points.clear();
        if (paramsWater.riverNoise.noiseEnabled)
        {
            tensorField.enableGlobalNoise(paramsWater.riverNoise.noiseAngle, paramsWater.riverNoise.noiseSize);
        }

        for (int i = 0; i < TRIES; ++i)
        {
            seed = getSeed(!coastlineMajor);
            river = extendStreamline(integrateStreamline(seed, !coastlineMajor));
            if (reachesEdges(river))
                break;
        }

        tensorField.sea = oldSea;
        tensorField.disableGlobalNoise();

        auto expandedNoisy = complexifyStreamline(PolygonUtil::resizeGeometry(river, paramsWater.riverSize, false));
        riverPoly = PolygonUtil::resizeGeometry(river, paramsWater.riverSize - paramsWater.riverBankSize, false);

        // ensure start off screen
        auto itOff = std::find_if(expandedNoisy.begin(), expandedNoisy.end(), [&](const CityModel::Vec2 &v)
                                  { return vectorOffScreen(v); });
        if (itOff != expandedNoisy.end())
        {
            std::rotate(expandedNoisy.begin(), itOff, expandedNoisy.end());
        }

        auto riverSplitPoly = getSeaPolygon(river);
        std::vector<CityModel::Vec2> road1, road2;
        for (const auto &v : expandedNoisy)
        {
            bool insideSea = PolygonUtil::insidePolygon(v, seaPoly);
            bool insideSplit = PolygonUtil::insidePolygon(v, riverSplitPoly);
            if (!insideSea && !vectorOffScreen(v) && insideSplit)
                road1.push_back(v);
            if (!insideSea && !vectorOffScreen(v) && !insideSplit)
                road2.push_back(v);
        }
        auto road1Simple = simplifyStreamline(road1);
        auto road2Simple = simplifyStreamline(road2);
        if (road1.empty() || road2.empty())
            return;
        if (road1[0].distanceToSquared(road2[0]) < road1[0].distanceToSquared(road2.back()))
        {
            std::reverse(road2Simple.begin(), road2Simple.end());
        }

        tensorField.river = CityModel::Polyline{road1Simple};
        riverSecondary = road2Simple;

        allStreamlinesSimple.push_back(road1Simple);
        grid(!coastlineMajor).addPolyline(road1);
        grid(!coastlineMajor).addPolyline(road2);
        streamlines(!coastlineMajor).push_back(road1);
        streamlines(!coastlineMajor).push_back(road2);
        allStreamlines.push_back(road1);
        allStreamlines.push_back(road2);
        RCG::DebugLog::printf("[WaterGen] createRiver done primary=%zu secondary=%zu\n",
                              tensorField.river.points.size(), riverSecondary.size());
    }

    std::vector<CityModel::Vec2> WaterGenerator::extendStreamline(std::vector<CityModel::Vec2> s)
    {
        if (s.size() < 2)
            return s;
        s.insert(s.begin(), s.front().clone().add(s.front().clone().sub(s[1]).setLength(params.dstep * 5)));
        s.push_back(s.back().clone().add(s.back().clone().sub(s[s.size() - 2]).setLength(params.dstep * 5)));
        return s;
    }

    bool WaterGenerator::reachesEdges(const std::vector<CityModel::Vec2> &s) const
    {
        if (s.empty())
            return false;
        return vectorOffScreen(s.front()) && vectorOffScreen(s.back());
    }

    bool WaterGenerator::vectorOffScreen(const CityModel::Vec2 &v) const
    {
        CityModel::Vec2 toOrigin = v.clone().sub(origin);
        return toOrigin.x <= 0 || toOrigin.y <= 0 ||
               toOrigin.x >= worldDimensions.x || toOrigin.y >= worldDimensions.y;
    }

    std::vector<CityModel::Vec2> WaterGenerator::getSeaPolygon(const std::vector<CityModel::Vec2> &polyline)
    {
        return PolygonUtil::lineRectanglePolygonIntersection(origin, worldDimensions, polyline);
    }

    std::vector<CityModel::Polyline> generate_water(const CityParams &params,
                                                    TensorField::TensorField &field,
                                                    const FieldIntegrator &integrator)
    {
        RCG::DebugLog::printf("[WaterGen] start seed=%u\n", params.seed);
        WaterParams wp;
        wp.dsep = params.water_dsep;
        wp.dtest = params.water_dtest;
        wp.dstep = params.water_dstep;
        wp.dlookahead = params.water_dlookahead;
        wp.dcirclejoin = params.water_dcirclejoin;
        wp.joinangle = params.water_joinangle;
        wp.pathIterations = params.water_pathIterations;
        wp.seedTries = params.water_seedTries;
        wp.simplifyTolerance = params.water_simplifyTolerance;
        wp.collideEarly = params.water_collideEarly;
        wp.coastNoise.noiseEnabled = params.water_coastNoiseEnabled;
        wp.coastNoise.noiseSize = params.water_coastNoiseSize;
        wp.coastNoise.noiseAngle = params.water_coastNoiseAngle;
        wp.riverNoise.noiseEnabled = params.water_riverNoiseEnabled;
        wp.riverNoise.noiseSize = params.water_riverNoiseSize;
        wp.riverNoise.noiseAngle = params.water_riverNoiseAngle;
        wp.riverBankSize = params.water_riverBankSize;
        wp.riverSize = params.water_riverSize;

        WaterGenerator gen(integrator, CityModel::Vec2{0, 0}, CityModel::Vec2{params.width, params.height}, wp, field, CityModel::RNG(params.seed));
        gen.createCoast();
        gen.createRiver();

        std::vector<CityModel::Polyline> out;
        if (!gen.seaPolygon().empty())
            out.push_back(CityModel::Polyline{gen.seaPolygon()});
        if (!gen.riverPolygon().empty())
            out.push_back(CityModel::Polyline{gen.riverPolygon()});
        // Add secondary river road as water feature for now.
        if (!gen.riverSecondaryRoad().empty())
            out.push_back(CityModel::Polyline{gen.riverSecondaryRoad()});
        RCG::DebugLog::printf("[WaterGen] done polylines=%zu\n", out.size());
        return out;
    }

} // namespace WaterGenerator
