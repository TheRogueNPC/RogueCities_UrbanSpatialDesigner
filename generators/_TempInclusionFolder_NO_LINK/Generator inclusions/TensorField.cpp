#include "TensorField.h"

#include <cmath>
#include <memory>
#include <algorithm>

#include "PolygonUtil.h"

namespace TensorField
{

    namespace
    {
        constexpr double PI = 3.14159265358979323846;

        // Simple deterministic pseudo-noise in [-1,1] using sine hashing.
        double hashNoise(double x, double y, unsigned int seed)
        {
            const double n = std::sin(x * 12.9898 + y * 78.233 + static_cast<double>(seed) * 1.234567);
            return std::sin(n * 43758.5453);
        }
    } // namespace

    // -------- Tensor --------
    Tensor::Tensor(double rIn, double m0In, double m1In)
        : r(rIn), m0(m0In), m1(m1In), thetaCache(0.0), thetaDirty(true) {}

    Tensor Tensor::fromAngle(double angle)
    {
        return Tensor(1.0, std::cos(angle * 4.0), std::sin(angle * 4.0));
    }

    Tensor Tensor::fromVector(const CityModel::Vec2 &v)
    {
        const double t1 = v.x * v.x - v.y * v.y;
        const double t2 = 2.0 * v.x * v.y;
        const double t3 = t1 * t1 - t2 * t2;
        const double t4 = 2.0 * t1 * t2;
        return Tensor(1.0, t3, t4);
    }

    Tensor Tensor::zero() { return Tensor(0.0, 0.0, 0.0); }

    Tensor &Tensor::add(const Tensor &other, bool smooth)
    {
        m0 = m0 * r + other.m0 * other.r;
        m1 = m1 * r + other.m1 * other.r;

        if (smooth)
        {
            r = std::hypot(m0, m1);
            if (r != 0.0)
            {
                m0 /= r;
                m1 /= r;
            }
        }
        else
        {
            r = 2.0;
        }

        thetaDirty = true;
        return *this;
    }

    Tensor &Tensor::scale(double s)
    {
        r *= s;
        thetaDirty = true;
        return *this;
    }

    Tensor &Tensor::rotate(double theta)
    {
        if (theta == 0.0)
            return *this;

        double newTheta = this->theta() + theta;
        if (newTheta < PI)
        {
            newTheta += PI;
        }
        if (newTheta >= PI)
        {
            newTheta -= PI;
        }

        m0 = std::cos(2.0 * newTheta) * r;
        m1 = std::sin(2.0 * newTheta) * r;
        thetaCache = newTheta;
        thetaDirty = false;
        return *this;
    }

    CityModel::Vec2 Tensor::major() const
    {
        if (r == 0.0)
            return {0.0, 0.0};
        const double ang = theta();
        return CityModel::Vec2{std::cos(ang), std::sin(ang)};
    }

    CityModel::Vec2 Tensor::minor() const
    {
        if (r == 0.0)
            return {0.0, 0.0};
        const double ang = theta() + PI / 2.0;
        return CityModel::Vec2{std::cos(ang), std::sin(ang)};
    }

    double Tensor::theta() const
    {
        if (!thetaDirty)
            return thetaCache;
        if (r == 0.0)
        {
            thetaCache = 0.0;
        }
        else
        {
            thetaCache = std::atan2(m1 / r, m0 / r) / 2.0;
        }
        thetaDirty = false;
        return thetaCache;
    }

    // -------- BasisField --------
    BasisField::BasisField(const CityModel::Vec2 &c, double s, double d)
        : centre(c), size(s), decay(d) {}

    Tensor BasisField::weightedTensor(const CityModel::Vec2 &p, bool smooth) const
    {
        Tensor t = tensorAt(p);
        return t.scale(weight(p, smooth));
    }

    double BasisField::weightAt(const CityModel::Vec2 &p, bool smooth) const
    {
        return weight(p, smooth);
    }

    double BasisField::weight(const CityModel::Vec2 &p, bool smooth) const
    {
        const double dx = p.x - centre.x;
        const double dy = p.y - centre.y;
        const double normDist = std::sqrt(dx * dx + dy * dy) / size;
        if (smooth)
        {
            return std::pow(normDist, -decay);
        }
        if (decay == 0.0 && normDist >= 1.0)
        {
            return 0.0;
        }
        return std::pow(std::max(0.0, 1.0 - normDist), decay);
    }

    double BasisField::weight_square(const CityModel::Vec2 &p, bool smooth) const
    {
        // L∞ distance (Chebyshev norm): max of absolute differences
        const double dx = std::abs(p.x - centre.x) / size;
        const double dy = std::abs(p.y - centre.y) / size;
        const double normDist = std::max(dx, dy);

        if (smooth)
        {
            return std::pow(normDist, -decay);
        }
        if (decay == 0.0 && normDist >= 1.0)
        {
            return 0.0;
        }
        return std::pow(std::max(0.0, 1.0 - normDist), decay);
    }

    // -------- GridField --------
    GridField::GridField(const CityModel::Vec2 &c, double size, double decay, double thetaIn)
        : BasisField(c, size, decay), theta(thetaIn) {}

    Tensor GridField::tensorAt(const CityModel::Vec2 & /*p*/) const
    {
        const double cosv = std::cos(2.0 * theta);
        const double sinv = std::sin(2.0 * theta);
        return Tensor(1.0, cosv, sinv);
    }

    // -------- RadialField --------
    RadialField::RadialField(const CityModel::Vec2 &c, double size, double decay)
        : BasisField(c, size, decay) {}

    Tensor RadialField::tensorAt(const CityModel::Vec2 &p) const
    {
        const double tx = p.x - centre.x;
        const double ty = p.y - centre.y;
        const double t1 = ty * ty - tx * tx;
        const double t2 = -2.0 * tx * ty;
        return Tensor(1.0, t1, t2);
    }

    // -------- SquareField --------
    SquareField::SquareField(const CityModel::Vec2 &c, double size, double decay)
        : BasisField(c, size, decay) {}

    Tensor SquareField::tensorAt(const CityModel::Vec2 &p) const
    {
        // Grid tensor: all points align to same orientation (0 degrees = horizontal lines)
        // This is identical to GridField with theta=0
        const double cosv = std::cos(0.0); // theta = 0
        const double sinv = std::sin(0.0);
        return Tensor(1.0, cosv, sinv);
    }

    Tensor SquareField::weightedTensor(const CityModel::Vec2 &p, bool smooth) const
    {
        Tensor t = tensorAt(p);
        return t.scale(weight_square(p, smooth));
    }

    double SquareField::weightAt(const CityModel::Vec2 &p, bool smooth) const
    {
        return weight_square(p, smooth);
    }

    // -------- DeltaField --------
    DeltaField::DeltaField(const CityModel::Vec2 &c, double size, double decay, DeltaTerminal t)
        : BasisField(c, size, decay), terminal(t) {}

    CityModel::Vec2 DeltaField::getTerminalPoint() const
    {
        switch (terminal)
        {
        case DeltaTerminal::Top:
            return CityModel::Vec2{centre.x, centre.y - size};
        case DeltaTerminal::BottomLeft:
            return CityModel::Vec2{centre.x - size, centre.y + size};
        case DeltaTerminal::BottomRight:
            return CityModel::Vec2{centre.x + size, centre.y + size};
        default:
            return centre;
        }
    }

    Tensor DeltaField::tensorAt(const CityModel::Vec2 &p) const
    {
        // Tensor major axis points toward the terminal point
        CityModel::Vec2 toTerminal = getTerminalPoint();
        toTerminal.sub(p);

        if (toTerminal.length() < 1e-6)
        {
            // At terminal: no preferred direction
            return Tensor::zero();
        }

        return Tensor::fromVector(toTerminal);
    }

    // -------- TensorField aggregate --------
    TensorField::TensorField(const TensorFieldConfig &cfg, unsigned int seedIn)
        : config(cfg), seed(seedIn)
    {
        smooth = cfg.smooth;
    }

    void TensorField::addGrid(const CityModel::Vec2 &centre, double size, double decay, double theta)
    {
        basisFields.emplace_back(std::make_unique<GridField>(centre, size, decay, theta));
    }

    void TensorField::addRadial(const CityModel::Vec2 &centre, double size, double decay)
    {
        basisFields.emplace_back(std::make_unique<RadialField>(centre, size, decay));
    }

    void TensorField::addSquare(const CityModel::Vec2 &centre, double size, double decay)
    {
        basisFields.emplace_back(std::make_unique<SquareField>(centre, size, decay));
    }

    void TensorField::addDelta(const CityModel::Vec2 &centre, double size, double decay, DeltaTerminal terminal)
    {
        basisFields.emplace_back(std::make_unique<DeltaField>(centre, size, decay, terminal));
    }

    void TensorField::addGridCorrective(const CityModel::Vec2 &centre, double size, double decay, double theta)
    {
        basisFields.emplace_back(std::make_unique<GridField>(centre, size, decay, theta));
    }

    void TensorField::clear()
    {
        basisFields.clear();
        parks.clear();
        sea.points.clear();
        river.points.clear();
    }

    void TensorField::enableGlobalNoise(double angleDeg, double size)
    {
        config.noise.globalNoise = true;
        config.noise.noiseAngleGlobal = angleDeg;
        config.noise.noiseSizeGlobal = size;
    }

    void TensorField::disableGlobalNoise()
    {
        config.noise.globalNoise = false;
    }

    Tensor TensorField::samplePoint(const CityModel::Vec2 &p) const
    {
        if (!onLand(p))
            return Tensor::zero();

        Tensor acc = Tensor::zero();
        if (basisFields.empty())
        {
            acc = Tensor::fromAngle(0.0); // default grid
        }
        else
        {
            for (const auto &f : basisFields)
            {
                acc.add(f->weightedTensor(p, smooth), smooth);
            }
        }

        // Park noise
        if (inParks(p))
        {
            const double rot = rotationalNoise(p, config.noise.noiseSizePark, config.noise.noiseAnglePark);
            acc.rotate(rot);
        }

        // Global noise
        if (config.noise.globalNoise)
        {
            const double rot = rotationalNoise(p, config.noise.noiseSizeGlobal, config.noise.noiseAngleGlobal);
            acc.rotate(rot);
        }

        return acc;
    }

    CityModel::Vec2 TensorField::evaluate(const CityModel::Vec2 &p, bool major) const
    {
        const Tensor t = samplePoint(p);
        return major ? t.major() : t.minor();
    }

    double TensorField::influenceAt(const CityModel::Vec2 &p, bool smooth) const
    {
        if (!onLand(p))
            return 0.0;
        double total = 0.0;
        for (const auto &f : basisFields)
        {
            total += f->weightAt(p, smooth);
        }
        return total;
    }

    double TensorField::rotationalNoise(const CityModel::Vec2 &p, double noiseSize, double noiseAngleDeg) const
    {
        if (noiseSize == 0.0)
            return 0.0;
        const double n = hashNoise(p.x / noiseSize, p.y / noiseSize, seed);
        return n * noiseAngleDeg * PI / 180.0;
    }

    bool TensorField::onLand(const CityModel::Vec2 &p) const
    {
        const bool inSea = PolygonUtil::insidePolygon(p, sea.points);
        if (ignoreRiver)
            return !inSea;
        return !inSea && !PolygonUtil::insidePolygon(p, river.points);
    }

    bool TensorField::inParks(const CityModel::Vec2 &p) const
    {
        for (const auto &poly : parks)
        {
            if (PolygonUtil::insidePolygon(p, poly.points))
                return true;
        }
        return false;
    }

    TensorField make_tensor_field(const CityParams &params)
    {
        TensorFieldConfig cfg;
        cfg.resolution = (params.width + params.height) * 0.05;
        cfg.smooth = false;
        cfg.noise.globalNoise = params.tf_globalNoise;
        cfg.noise.noiseSizePark = params.tf_noiseSizePark;
        cfg.noise.noiseAnglePark = params.tf_noiseAnglePark;
        cfg.noise.noiseSizeGlobal = params.tf_noiseSizeGlobal;
        cfg.noise.noiseAngleGlobal = params.tf_noiseAngleGlobal;

        TensorField field(cfg, params.seed);
        // Basic default: one grid and one radial to mimic UI start.
        field.addGrid(CityModel::Vec2{params.width * 0.5, params.height * 0.5},
                      std::min(params.width, params.height) * 0.75,
                      1.0,
                      0.0);
        field.addRadial(CityModel::Vec2{params.width * 0.5, params.height * 0.5},
                        std::min(params.width, params.height) * 0.5,
                        1.0);
        return field;
    }

} // namespace TensorField
