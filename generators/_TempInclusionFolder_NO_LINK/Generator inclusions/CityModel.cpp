#include "CityModel.h"

#include <cmath>
#include <limits>

#include "AxiomInput.h"
#include "RoadGenerator.h"
#include "DistrictGenerator.h"
#include "LotGenerator.h"
#include "SiteGenerator.h"
#include "TensorField.h"
#include "WaterGenerator.h"
#include "Integrator.h"

namespace CityModel
{

    // -------- Vec2 methods --------
    Vec2 &Vec2::add(const Vec2 &v)
    {
        x += v.x;
        y += v.y;
        return *this;
    }

    Vec2 &Vec2::sub(const Vec2 &v)
    {
        x -= v.x;
        y -= v.y;
        return *this;
    }

    Vec2 &Vec2::multiply(const Vec2 &v)
    {
        x *= v.x;
        y *= v.y;
        return *this;
    }

    Vec2 &Vec2::multiply(double s)
    {
        x *= s;
        y *= s;
        return *this;
    }

    Vec2 &Vec2::divide(const Vec2 &v)
    {
        if (v.x != 0.0)
            x /= v.x;
        if (v.y != 0.0)
            y /= v.y;
        return *this;
    }

    Vec2 &Vec2::divide(double s)
    {
        if (s != 0.0)
        {
            x /= s;
            y /= s;
        }
        return *this;
    }

    Vec2 Vec2::negated() const
    {
        return Vec2{-x, -y};
    }

    Vec2 Vec2::clone() const
    {
        return Vec2{x, y};
    }

    Vec2 &Vec2::set(const Vec2 &v)
    {
        x = v.x;
        y = v.y;
        return *this;
    }

    Vec2 &Vec2::setLength(double len)
    {
        const double l = length();
        if (l > 0.0)
        {
            const double s = len / l;
            x *= s;
            y *= s;
        }
        return *this;
    }

    Vec2 &Vec2::normalize()
    {
        const double l = length();
        if (l > 0.0)
        {
            x /= l;
            y /= l;
        }
        return *this;
    }

    Vec2 &Vec2::rotateAround(const Vec2 &center, double angle)
    {
        const double cosA = std::cos(angle);
        const double sinA = std::sin(angle);
        const double rx = x - center.x;
        const double ry = y - center.y;
        x = rx * cosA - ry * sinA + center.x;
        y = rx * sinA + ry * cosA + center.y;
        return *this;
    }

    double Vec2::dot(const Vec2 &v) const { return x * v.x + y * v.y; }
    double Vec2::cross(const Vec2 &v) const { return x * v.y - y * v.x; }
    double Vec2::length() const { return std::sqrt(x * x + y * y); }
    double Vec2::lengthSquared() const { return x * x + y * y; }
    double Vec2::angle() const { return std::atan2(y, x); }
    double Vec2::distanceTo(const Vec2 &v) const { return std::sqrt(distanceToSquared(v)); }
    double Vec2::distanceToSquared(const Vec2 &v) const
    {
        const double dx = x - v.x;
        const double dy = y - v.y;
        return dx * dx + dy * dy;
    }
    bool Vec2::equals(const Vec2 &v) const { return x == v.x && y == v.y; }

    // -------- RNG --------
    RNG::RNG(unsigned int seed) : gen(seed), dist01(0.0, 1.0) {}

    double RNG::uniform() { return dist01(gen); }
    double RNG::uniform(double max) { return dist01(gen) * max; }
    double RNG::uniform(double min, double max) { return min + dist01(gen) * (max - min); }
    int RNG::uniformInt(int min, int max)
    {
        std::uniform_int_distribution<int> dist(min, max);
        return dist(gen);
    }

    // -------- Utility helpers --------
    double distance(const Vec2 &a, const Vec2 &b)
    {
        const double dx = a.x - b.x;
        const double dy = a.y - b.y;
        return std::sqrt(dx * dx + dy * dy);
    }

    Vec2 lerp(const Vec2 &a, const Vec2 &b, double t)
    {
        return Vec2{a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
    }

    double angleBetween(const Vec2 &v1, const Vec2 &v2)
    {
        constexpr double PI = 3.14159265358979323846;
        double diff = v1.angle() - v2.angle();
        if (diff > PI)
            diff -= 2 * PI;
        else if (diff <= -PI)
            diff += 2 * PI;
        return diff;
    }

    bool isLeft(const Vec2 &linePoint, const Vec2 &lineDir, const Vec2 &point)
    {
        const Vec2 perp{lineDir.y, -lineDir.x};
        const Vec2 diff = Vec2{point.x - linePoint.x, point.y - linePoint.y};
        return diff.dot(perp) < 0.0;
    }

    // -------- Pipeline entry point --------
    City generate_city(const CityParams &params,
                       const std::vector<AxiomInput> &axioms,
                       const UserPlacedInputs &user_inputs)
    {
        City city;
        city.bounds.min = {0.0, 0.0};
        city.bounds.max = {params.width, params.height};

        auto field = TensorField::make_tensor_field(params);
        RK4Integrator waterIntegrator(field, params.water_dstep);
        city.water = WaterGenerator::generate_water(params, field, waterIntegrator);
        
        // Phase 0: Roads
        if (params.phase_enabled[static_cast<int>(GeneratorPhase::Roads)])
        {
            printf("[Pipeline] Generating roads...\n");
            RoadGenerator::generate_roads(params, field, city.water, city);
        }
        else
        {
            printf("[Pipeline] Roads phase disabled\n");
        }

        // Phase 1: Districts
        DistrictGenerator::DistrictField district_field;
        if (params.phase_enabled[static_cast<int>(GeneratorPhase::Districts)])
        {
            printf("[Pipeline] Generating districts...\n");
            DistrictGenerator::generate(params, axioms, city, district_field, DistrictGenerator::Settings{}, &field);
            DistrictGenerator::clip_roads_to_districts(city, district_field);
        }
        else
        {
            printf("[Pipeline] Districts phase disabled\n");
        }

        // Phase 2: Blocks (handled in LotGenerator)
        // Phase 3: Lots
        if (params.phase_enabled[static_cast<int>(GeneratorPhase::Lots)])
        {
            printf("[Pipeline] Generating lots...\n");
            LotGenerator::generate(params, axioms, city.districts, district_field, field, city, user_inputs);
        }
        else
        {
            printf("[Pipeline] Lots phase disabled\n");
        }

        // Phase 4: Buildings
        if (params.phase_enabled[static_cast<int>(GeneratorPhase::Buildings)])
        {
            printf("[Pipeline] Generating buildings...\n");
            SiteGenerator::generate(params, city, user_inputs);
        }
        else
        {
            printf("[Pipeline] Buildings phase disabled\n");
        }

        return city;
    }

} // namespace CityModel
