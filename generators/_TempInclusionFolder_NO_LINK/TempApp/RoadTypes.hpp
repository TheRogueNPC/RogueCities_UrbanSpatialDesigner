#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace CityModel
{
    enum class RoadType
    {
        Highway,
        Arterial,
        Avenue,
        Boulevard,
        Street,
        Lane,
        Alleyway,
        CulDeSac,
        Drive,
        Driveway,
        M_Major,
        M_Minor,
        Count
    };

    constexpr std::size_t road_type_count = static_cast<std::size_t>(RoadType::Count);

    inline bool is_user_road_type(RoadType type)
    {
        return type == RoadType::M_Major || type == RoadType::M_Minor;
    }

    inline bool is_major_group(RoadType type)
    {
        return type == RoadType::Highway || type == RoadType::Arterial || type == RoadType::Avenue || type == RoadType::Boulevard;
    }

    inline constexpr std::array<RoadType, 10> generated_road_order = {
        RoadType::Highway,
        RoadType::Arterial,
        RoadType::Avenue,
        RoadType::Boulevard,
        RoadType::Street,
        RoadType::Lane,
        RoadType::Alleyway,
        RoadType::CulDeSac,
        RoadType::Drive,
        RoadType::Driveway};

    inline const char *road_type_label(RoadType type)
    {
        switch (type)
        {
        case RoadType::Highway:
            return "Highway";
        case RoadType::Arterial:
            return "Arterial";
        case RoadType::Avenue:
            return "Avenue";
        case RoadType::Boulevard:
            return "Boulevard";
        case RoadType::Street:
            return "Street";
        case RoadType::Lane:
            return "Lane";
        case RoadType::Alleyway:
            return "Alleyway";
        case RoadType::CulDeSac:
            return "CulDeSac";
        case RoadType::Drive:
            return "Drive";
        case RoadType::Driveway:
            return "Driveway";
        case RoadType::M_Major:
            return "M_Major";
        case RoadType::M_Minor:
            return "M_Minor";
        default:
            return "Unknown";
        }
    }

    inline const char *road_type_key(RoadType type)
    {
        switch (type)
        {
        case RoadType::Highway:
            return "highway";
        case RoadType::Arterial:
            return "arterial";
        case RoadType::Avenue:
            return "avenue";
        case RoadType::Boulevard:
            return "boulevard";
        case RoadType::Street:
            return "street";
        case RoadType::Lane:
            return "lane";
        case RoadType::Alleyway:
            return "alleyway";
        case RoadType::CulDeSac:
            return "cul_de_sac";
        case RoadType::Drive:
            return "drive";
        case RoadType::Driveway:
            return "driveway";
        case RoadType::M_Major:
            return "m_major";
        case RoadType::M_Minor:
            return "m_minor";
        default:
            return "unknown";
        }
    }

    inline constexpr std::size_t road_type_index(RoadType type)
    {
        return static_cast<std::size_t>(type);
    }

    inline constexpr uint32_t road_type_bit(RoadType type)
    {
        return (1u << static_cast<uint32_t>(type));
    }

} // namespace CityModel
