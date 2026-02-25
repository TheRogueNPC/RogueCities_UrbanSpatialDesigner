#pragma once

#include "TensorField.h"
#include "CityModel.h"
#include "CityParams.h"

class FieldIntegrator
{
public:
    explicit FieldIntegrator(const TensorField::TensorField &field) : field(field) {}
    virtual ~FieldIntegrator() = default;
    virtual CityModel::Vec2 integrate(const CityModel::Vec2 &point, bool major) const = 0;
    bool onLand(const CityModel::Vec2 &point) const { return field.onLand(point); }
    double influenceAt(const CityModel::Vec2 &point) const { return field.influenceAt(point); }

protected:
    const TensorField::TensorField &field;
};

class EulerIntegrator : public FieldIntegrator
{
public:
    EulerIntegrator(const TensorField::TensorField &field, double dstep);
    CityModel::Vec2 integrate(const CityModel::Vec2 &point, bool major) const override;

private:
    double dstep;
};

class RK4Integrator : public FieldIntegrator
{
public:
    RK4Integrator(const TensorField::TensorField &field, double dstep);
    CityModel::Vec2 integrate(const CityModel::Vec2 &point, bool major) const override;

private:
    double dstep;
};
