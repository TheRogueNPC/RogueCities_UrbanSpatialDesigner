#pragma once

#include <memory>
#include <vector>

#include "CityModel.h"
#include "CityParams.h"

namespace TensorField
{

    // --- Tensor math ---
    struct Tensor
    {
        double r{0.0};
        double m0{0.0};
        double m1{0.0};
        mutable double thetaCache{0.0};
        mutable bool thetaDirty{false};

        Tensor() = default;
        Tensor(double rIn, double m0In, double m1In);

        static Tensor fromAngle(double angle);
        static Tensor fromVector(const CityModel::Vec2 &v);
        static Tensor zero();

        Tensor &add(const Tensor &other, bool smooth);
        Tensor &scale(double s);
        Tensor &rotate(double theta); // radians

        CityModel::Vec2 major() const;
        CityModel::Vec2 minor() const;

    private:
        double theta() const;
    };

    // --- Basis fields ---
    class BasisField
    {
    public:
        virtual ~BasisField() = default;
        virtual Tensor tensorAt(const CityModel::Vec2 &p) const = 0;
        virtual Tensor weightedTensor(const CityModel::Vec2 &p, bool smooth) const;
        virtual double weightAt(const CityModel::Vec2 &p, bool smooth) const;

        void setCentre(const CityModel::Vec2 &c) { centre = c; }
        CityModel::Vec2 getCentre() const { return centre; }

    protected:
        BasisField(const CityModel::Vec2 &centre, double size, double decay);
        double weight(const CityModel::Vec2 &p, bool smooth) const;
        double weight_square(const CityModel::Vec2 &p, bool smooth) const;

        CityModel::Vec2 centre;
        double size;
        double decay;
    };

    class GridField : public BasisField
    {
    public:
        GridField(const CityModel::Vec2 &centre, double size, double decay, double theta);
        void setTheta(double theta) { this->theta = theta; }
        Tensor tensorAt(const CityModel::Vec2 &p) const override;

    private:
        double theta;
    };

    class RadialField : public BasisField
    {
    public:
        RadialField(const CityModel::Vec2 &centre, double size, double decay);
        Tensor tensorAt(const CityModel::Vec2 &p) const override;
    };

    class SquareField : public BasisField
    {
    public:
        SquareField(const CityModel::Vec2 &centre, double size, double decay);
        Tensor tensorAt(const CityModel::Vec2 &p) const override;
        Tensor weightedTensor(const CityModel::Vec2 &p, bool smooth) const override;
        double weightAt(const CityModel::Vec2 &p, bool smooth) const override;
    };

    // Terminal points for DeltaField
    enum class DeltaTerminal
    {
        Top = 0,
        BottomLeft = 1,
        BottomRight = 2
    };

    class DeltaField : public BasisField
    {
    public:
        DeltaField(const CityModel::Vec2 &centre, double size, double decay, DeltaTerminal terminal);
        void setTerminal(DeltaTerminal t) { this->terminal = t; }
        DeltaTerminal getTerminal() const { return terminal; }
        Tensor tensorAt(const CityModel::Vec2 &p) const override;

    private:
        DeltaTerminal terminal;
        CityModel::Vec2 getTerminalPoint() const;
    };

    // --- Noise parameters ---
    struct NoiseParams
    {
        bool globalNoise{false};
        double noiseSizePark{50.0};
        double noiseAnglePark{0.0}; // degrees
        double noiseSizeGlobal{100.0};
        double noiseAngleGlobal{0.0}; // degrees
    };

    // --- Tensor field aggregate ---
    struct TensorFieldConfig
    {
        double resolution{50.0};
        NoiseParams noise;
        bool smooth{false};
    };

    class TensorField
    {
    public:
        explicit TensorField(const TensorFieldConfig &cfg, unsigned int seed = 1);

        void addGrid(const CityModel::Vec2 &centre, double size, double decay, double theta);
        void addRadial(const CityModel::Vec2 &centre, double size, double decay);
        void addSquare(const CityModel::Vec2 &centre, double size, double decay);
        void addDelta(const CityModel::Vec2 &centre, double size, double decay, DeltaTerminal terminal);
        void addGridCorrective(const CityModel::Vec2 &centre, double size, double decay, double theta);
        void clear();

        void enableGlobalNoise(double angleDeg, double size);
        void disableGlobalNoise();

        Tensor samplePoint(const CityModel::Vec2 &p) const;
        CityModel::Vec2 evaluate(const CityModel::Vec2 &p, bool major) const;
        double influenceAt(const CityModel::Vec2 &p, bool smooth = false) const;
        bool onLand(const CityModel::Vec2 &p) const;
        bool inParks(const CityModel::Vec2 &p) const;

        // Masks
        std::vector<CityModel::Polyline> parks; // polygons
        CityModel::Polyline sea;
        CityModel::Polyline river;
        bool ignoreRiver{false};
        bool smooth{false};

    private:
        double rotationalNoise(const CityModel::Vec2 &p, double noiseSize, double noiseAngleDeg) const;

        TensorFieldConfig config;
        unsigned int seed;
        std::vector<std::unique_ptr<BasisField>> basisFields;
    };

    TensorField make_tensor_field(const CityParams &params);

} // namespace TensorField
