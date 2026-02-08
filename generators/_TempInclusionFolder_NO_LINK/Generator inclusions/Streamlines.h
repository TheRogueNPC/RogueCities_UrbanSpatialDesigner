#pragma once

#include <vector>

#include "CityModel.h"
#include "GridStorage.h"
#include "Integrator.h"

struct StreamlineIntegration;

struct StreamlineParams
{
    double dsep{20.0};
    double dtest{15.0};
    double dstep{1.0};
    double dcirclejoin{5.0};
    double dlookahead{40.0};
    double joinangle{0.1};
    int pathIterations{1000};
    int seedTries{300};
    double simplifyTolerance{0.5};
    double collideEarly{0.0};
};

class StreamlineGenerator
{
public:
    StreamlineGenerator(const FieldIntegrator &integrator,
                        const CityModel::Vec2 &origin,
                        const CityModel::Vec2 &worldDimensions,
                        const StreamlineParams &params,
                        CityModel::RNG rng);

    void clearStreamlines();
    std::vector<CityModel::Vec2> integrateStreamline(const CityModel::Vec2 &seed, bool major);
    CityModel::Vec2 getSeed(bool major);
    bool isValidSample(bool major, const CityModel::Vec2 &point, double dSq, bool bothGrids = false) const;

    const std::vector<std::vector<CityModel::Vec2>> &getAllStreamlines() const { return allStreamlines; }
    std::vector<std::vector<CityModel::Vec2>> &streamlines(bool major) { return major ? streamlinesMajor : streamlinesMinor; }
    GridStorage &grid(bool major) { return major ? majorGrid : minorGrid; }
    const GridStorage &grid(bool major) const { return major ? majorGrid : minorGrid; }

    std::vector<std::vector<CityModel::Vec2>> allStreamlinesSimple;
    std::vector<std::vector<CityModel::Vec2>> streamlinesMajor;
    std::vector<std::vector<CityModel::Vec2>> streamlinesMinor;
    std::vector<std::vector<CityModel::Vec2>> allStreamlines;

    // Helpers
    std::vector<CityModel::Vec2> simplifyStreamline(const std::vector<CityModel::Vec2> &s) const;
    std::vector<CityModel::Vec2> complexifyStreamline(const std::vector<CityModel::Vec2> &s) const;
    std::vector<CityModel::Vec2> complexifySegment(const CityModel::Vec2 &v1, const CityModel::Vec2 &v2) const;
    std::vector<CityModel::Vec2> pointsBetween(const CityModel::Vec2 &v1, const CityModel::Vec2 &v2, double dstep) const;

    CityModel::Vec2 samplePoint();

protected:
    bool pointInBounds(const CityModel::Vec2 &v) const;
    bool streamlineTurned(const CityModel::Vec2 &seed,
                          const CityModel::Vec2 &originalDir,
                          const CityModel::Vec2 &point,
                          const CityModel::Vec2 &direction) const;
    void streamlineIntegrationStep(struct StreamlineIntegration &params, bool major, bool collideBoth);

    const FieldIntegrator &integrator;
    CityModel::Vec2 origin;
    CityModel::Vec2 worldDimensions;
    StreamlineParams params;
    StreamlineParams paramsSq;
    GridStorage majorGrid;
    GridStorage minorGrid;
    int nStreamlineStep{0};
    int nStreamlineLookBack{0};
    double dcollideselfSq{0.0};
    CityModel::RNG rng;
};
