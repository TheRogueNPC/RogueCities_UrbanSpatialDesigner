#pragma once

#include <vector>

#include "CityModel.h"
#include "CityParams.h"
#include "Streamlines.h"
#include "TensorField.h"

namespace WaterGenerator
{

    struct NoiseStreamlineParams
    {
        bool noiseEnabled{true};
        double noiseSize{30.0};
        double noiseAngle{20.0};
    };

    struct WaterParams : public StreamlineParams
    {
        NoiseStreamlineParams coastNoise;
        NoiseStreamlineParams riverNoise;
        double riverBankSize{10.0};
        double riverSize{30.0};
    };

    class WaterGenerator : public StreamlineGenerator
    {
    public:
        WaterGenerator(const FieldIntegrator &integrator,
                       const CityModel::Vec2 &origin,
                       const CityModel::Vec2 &worldDimensions,
                       const WaterParams &params,
                       TensorField::TensorField &tensorField,
                       CityModel::RNG rng);

        void createCoast();
        void createRiver();

        const std::vector<CityModel::Vec2> &coastline() const { return coastlineLine; }
        const std::vector<CityModel::Vec2> &seaPolygon() const { return seaPoly; }
        const std::vector<CityModel::Vec2> &riverPolygon() const { return riverPoly; }
        const std::vector<CityModel::Vec2> &riverSecondaryRoad() const { return riverSecondary; }

    private:
        std::vector<CityModel::Vec2> extendStreamline(std::vector<CityModel::Vec2> s);
        bool reachesEdges(const std::vector<CityModel::Vec2> &s) const;
        bool vectorOffScreen(const CityModel::Vec2 &v) const;
        std::vector<CityModel::Vec2> getSeaPolygon(const std::vector<CityModel::Vec2> &polyline);

        WaterParams paramsWater;
        TensorField::TensorField &tensorField;
        std::vector<CityModel::Vec2> coastlineLine;
        std::vector<CityModel::Vec2> seaPoly;
        std::vector<CityModel::Vec2> riverPoly;
        std::vector<CityModel::Vec2> riverSecondary;
        bool coastlineMajor{true};
        const int TRIES = 100;
    };

    std::vector<CityModel::Polyline> generate_water(const CityParams &params,
                                                    TensorField::TensorField &field,
                                                    const FieldIntegrator &integrator);

} // namespace WaterGenerator
