#include "Integrator.h"

namespace
{
    CityModel::Vec2 sampleDir(const TensorField::TensorField &field, const CityModel::Vec2 &p, bool major)
    {
        return field.evaluate(p, major);
    }
}

EulerIntegrator::EulerIntegrator(const TensorField::TensorField &f, double step)
    : FieldIntegrator(f), dstep(step) {}

CityModel::Vec2 EulerIntegrator::integrate(const CityModel::Vec2 &point, bool major) const
{
    CityModel::Vec2 dir = sampleDir(field, point, major);
    dir.multiply(dstep);
    return dir;
}

RK4Integrator::RK4Integrator(const TensorField::TensorField &f, double step)
    : FieldIntegrator(f), dstep(step) {}

CityModel::Vec2 RK4Integrator::integrate(const CityModel::Vec2 &point, bool major) const
{
    const CityModel::Vec2 k1 = sampleDir(field, point, major);
    CityModel::Vec2 mid = point.clone().add(CityModel::Vec2{k1.x * dstep * 0.5, k1.y * dstep * 0.5});
    const CityModel::Vec2 k2 = sampleDir(field, mid, major);
    mid = point.clone().add(CityModel::Vec2{k2.x * dstep * 0.5, k2.y * dstep * 0.5});
    const CityModel::Vec2 k3 = sampleDir(field, mid, major);
    mid = point.clone().add(CityModel::Vec2{k3.x * dstep, k3.y * dstep});
    const CityModel::Vec2 k4 = sampleDir(field, mid, major);

    CityModel::Vec2 sum{k1.x + 2 * k2.x + 2 * k3.x + k4.x,
                        k1.y + 2 * k2.y + 2 * k3.y + k4.y};
    sum.multiply(dstep / 6.0);
    return sum;
}
