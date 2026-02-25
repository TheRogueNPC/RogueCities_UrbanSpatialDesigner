#include "CityParams.h"

CityParams make_default_city_params()
{
    CityParams params;
    params.width = 1000.0;
    params.height = 1000.0;
    params.seed = 1u;
    params.randomizeSites = false;
    params.bufferUtilityChance = 0.35;

    // Tensor noise defaults already set in struct.

    // Water defaults aligned to UI.
    params.water_pathIterations = 10000;
    params.water_simplifyTolerance = 10.0;

    // Main/major/minor inherit struct defaults.

    auto set_type = [&](CityModel::RoadType type,
                        double dsep,
                        double dtest,
                        double dlookahead,
                        bool majorDir,
                        bool pruneDangling,
                        bool enabled)
    {
        auto &tp = params.road_type_params[CityModel::road_type_index(type)];
        tp.dsep = dsep;
        tp.dtest = dtest;
        tp.dstep = 1.0;
        tp.dlookahead = dlookahead;
        tp.dcirclejoin = 5.0;
        tp.joinangle = 0.1;
        tp.pathIterations = 1000;
        tp.seedTries = 300;
        tp.simplifyTolerance = 0.5;
        tp.collideEarly = 0.0;
        tp.majorDirection = majorDir;
        tp.pruneDangling = pruneDangling;
        tp.enabled = enabled;
    };

    set_type(CityModel::RoadType::Highway, 600.0, 250.0, 600.0, true, true, true);
    set_type(CityModel::RoadType::Arterial, 350.0, 180.0, 400.0, true, true, true);
    set_type(CityModel::RoadType::Avenue, 250.0, 140.0, 300.0, true, true, true);
    set_type(CityModel::RoadType::Boulevard, 200.0, 120.0, 240.0, true, true, true);
    set_type(CityModel::RoadType::Street, 120.0, 60.0, 140.0, false, true, true);
    set_type(CityModel::RoadType::Lane, 80.0, 45.0, 100.0, false, true, true);
    set_type(CityModel::RoadType::Alleyway, 50.0, 30.0, 70.0, false, true, true);
    set_type(CityModel::RoadType::CulDeSac, 40.0, 25.0, 50.0, false, false, true);
    set_type(CityModel::RoadType::Drive, 60.0, 35.0, 80.0, false, true, true);
    set_type(CityModel::RoadType::Driveway, 25.0, 15.0, 30.0, false, false, false);

    auto set_mask = [&](CityModel::RoadType type, uint32_t mask)
    {
        params.road_type_params[CityModel::road_type_index(type)].allowIntersectionsMask = mask;
    };

    const uint32_t all = 0xFFFFFFFFu;
    const uint32_t allow_highway = CityModel::road_type_bit(CityModel::RoadType::Highway) |
                                   CityModel::road_type_bit(CityModel::RoadType::Arterial) |
                                   CityModel::road_type_bit(CityModel::RoadType::Avenue) |
                                   CityModel::road_type_bit(CityModel::RoadType::Boulevard) |
                                   CityModel::road_type_bit(CityModel::RoadType::Street) |
                                   CityModel::road_type_bit(CityModel::RoadType::Drive);
    set_mask(CityModel::RoadType::Highway, allow_highway);
    set_mask(CityModel::RoadType::Arterial, all);
    set_mask(CityModel::RoadType::Avenue, all);
    set_mask(CityModel::RoadType::Boulevard, all);
    set_mask(CityModel::RoadType::Street, all);
    set_mask(CityModel::RoadType::Lane, all);
    set_mask(CityModel::RoadType::Alleyway, all);
    set_mask(CityModel::RoadType::CulDeSac, all);
    set_mask(CityModel::RoadType::Drive, all);
    set_mask(CityModel::RoadType::Driveway, all);

    params.road_type_params[CityModel::road_type_index(CityModel::RoadType::Driveway)].requireDeadEnd = true;
    params.road_type_params[CityModel::road_type_index(CityModel::RoadType::CulDeSac)].requireDeadEnd = true;
    params.road_type_params[CityModel::road_type_index(CityModel::RoadType::Highway)].allowDeadEnds = false;

    params.block_barrier.fill(true);
    params.block_closure.fill(true);
    params.block_closure[CityModel::road_type_index(CityModel::RoadType::Highway)] = false;
    params.block_closure[CityModel::road_type_index(CityModel::RoadType::Arterial)] = false;

    return params;
}
