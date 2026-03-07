pub mod bindings;

use bindings::*;
use spacetimedb_sdk::DbContext;
use std::ffi::CStr;
use std::os::raw::c_char;
use std::sync::{Arc, Mutex};
use tokio::runtime::Runtime;
use std::thread;

lazy_static::lazy_static! {
    static ref DB_CONN: Mutex<Option<Arc<DbConnection>>> = Mutex::new(None);
}

// ====================================================================
// HELPER: Convert interleaved flat [x0,y0,x1,y1,...] to Vec<Vec2>
// ====================================================================
unsafe fn flat_to_vec2_list(ptr: *const f64, count: u32) -> Vec<Vec2> {
    let mut result = Vec::with_capacity(count as usize);
    if !ptr.is_null() {
        for i in 0..count as usize {
            result.push(Vec2 {
                x: *ptr.add(i * 2),
                y: *ptr.add(i * 2 + 1),
            });
        }
    }
    result
}

// ====================================================================
// CONNECTION
// ====================================================================

#[no_mangle]
pub extern "C" fn rc_db_connect(
    database_name: *const c_char,
    server_url: *const c_char,
) -> bool {
    let db_name_str = unsafe {
        if database_name.is_null() { return false; }
        CStr::from_ptr(database_name).to_string_lossy().into_owned()
    };

    let server_url_str = unsafe {
        if server_url.is_null() { return false; }
        CStr::from_ptr(server_url).to_string_lossy().into_owned()
    };

    let (tx, rx) = std::sync::mpsc::channel();

    thread::spawn(move || {
        let rt = Runtime::new().unwrap();
        rt.block_on(async {
            let builder = DbConnection::builder()
                .with_uri(server_url_str.as_str())
                .with_database_name(db_name_str.clone());
            
            let conn = match builder.build() {
                Ok(c) => Arc::new(c),
                Err(e) => {
                    eprintln!("Failed to connect to SpacetimeDB: {:?}", e);
                    tx.send(None).unwrap();
                    return;
                }
            };
            
            tx.send(Some(Arc::clone(&conn))).unwrap();
            
            if let Err(e) = conn.run_async().await {
                eprintln!("SpacetimeDB disconnect: {:?}", e);
            }
        });
    });

    match rx.recv() {
        Ok(Some(conn)) => {
            let mut global_conn = DB_CONN.lock().unwrap();
            *global_conn = Some(conn);
            true
        }
        _ => false,
    }
}

#[no_mangle]
pub extern "C" fn rc_db_disconnect() {
    let mut global_conn = DB_CONN.lock().unwrap();
    if let Some(conn) = global_conn.take() {
        let _ = conn.disconnect();
    }
}

// ====================================================================
// AXIOM REDUCERS
// ====================================================================

#[no_mangle]
pub extern "C" fn rc_db_place_axiom(
    axiom_type: u32,
    pos_x: f64,
    pos_y: f64,
    radius: f64,
    theta: f64,
    decay: f64,
) {
    if let Some(conn) = DB_CONN.lock().unwrap().as_ref() {
        let atype = match axiom_type {
            0 => AxiomType::Organic,
            1 => AxiomType::Grid,
            2 => AxiomType::Radial,
            3 => AxiomType::Hexagonal,
            4 => AxiomType::Stem,
            5 => AxiomType::LooseGrid,
            6 => AxiomType::Suburban,
            7 => AxiomType::Superblock,
            8 => AxiomType::Linear,
            9 => AxiomType::GridCorrective,
            _ => AxiomType::Organic,
        };
        let _ = conn.reducers.place_axiom(atype, pos_x, pos_y, radius, theta, decay);
    }
}

#[no_mangle]
pub extern "C" fn rc_db_clear_axioms() {
    if let Some(conn) = DB_CONN.lock().unwrap().as_ref() {
        let _ = conn.reducers.clear_axioms();
    }
}

// ====================================================================
// BULK PUSH REDUCERS (Headless Compute Worker → SpacetimeDB)
// ====================================================================

#[no_mangle]
pub extern "C" fn rc_db_push_road(
    id: u32,
    road_type: u32,
    source_axiom_id: i32,
    points_xy: *const f64,
    num_points: u32,
    layer_id: i32,
    has_grade_sep: bool,
    has_signal: bool,
    has_crosswalk: bool,
    generation_serial: u64,
) {
    if let Some(conn) = DB_CONN.lock().unwrap().as_ref() {
        let rtype = match road_type {
            0 => RoadType::Highway,
            1 => RoadType::Arterial,
            2 => RoadType::Avenue,
            3 => RoadType::Boulevard,
            4 => RoadType::Street,
            5 => RoadType::Lane,
            6 => RoadType::Alleyway,
            7 => RoadType::CulDeSac,
            8 => RoadType::Drive,
            9 => RoadType::Driveway,
            10 => RoadType::MMajor,
            11 => RoadType::MMinor,
            _ => RoadType::Street,
        };
        let points = unsafe { flat_to_vec2_list(points_xy, num_points) };
        let _ = conn.reducers.push_road(
            id, rtype, source_axiom_id, points, layer_id,
            has_grade_sep, has_signal, has_crosswalk, generation_serial,
        );
    }
}

#[no_mangle]
pub extern "C" fn rc_db_push_district(
    id: u32,
    district_type: u32,
    primary_axiom_id: i32,
    border_xy: *const f64,
    num_border_pts: u32,
    budget: f32,
    population: u32,
    desirability: f32,
    avg_elevation: f32,
    generation_serial: u64,
) {
    if let Some(conn) = DB_CONN.lock().unwrap().as_ref() {
        let dtype = match district_type {
            0 => DistrictType::Mixed,
            1 => DistrictType::Residential,
            2 => DistrictType::Commercial,
            3 => DistrictType::Civic,
            4 => DistrictType::Industrial,
            _ => DistrictType::Mixed,
        };
        let border = unsafe { flat_to_vec2_list(border_xy, num_border_pts) };
        let _ = conn.reducers.push_district(
            id, dtype, primary_axiom_id, border,
            budget, population, desirability, avg_elevation, generation_serial,
        );
    }
}

#[no_mangle]
pub extern "C" fn rc_db_push_lot(
    id: u32,
    district_id: u32,
    lot_type: u32,
    centroid_x: f64,
    centroid_y: f64,
    boundary_xy: *const f64,
    num_boundary_pts: u32,
    area: f32,
    access: f32,
    exposure: f32,
    serviceability: f32,
    privacy: f32,
    generation_serial: u64,
) {
    if let Some(conn) = DB_CONN.lock().unwrap().as_ref() {
        let ltype = match lot_type {
            0 => LotType::None,
            1 => LotType::Residential,
            2 => LotType::RowhomeCompact,
            3 => LotType::RetailStrip,
            4 => LotType::MixedUse,
            5 => LotType::LogisticsIndustrial,
            6 => LotType::CivicCultural,
            7 => LotType::LuxuryScenic,
            8 => LotType::BufferStrip,
            _ => LotType::None,
        };
        let boundary = unsafe { flat_to_vec2_list(boundary_xy, num_boundary_pts) };
        let _ = conn.reducers.push_lot(
            id, district_id, ltype, centroid_x, centroid_y, boundary,
            area, access, exposure, serviceability, privacy, generation_serial,
        );
    }
}

#[no_mangle]
pub extern "C" fn rc_db_push_building(
    id: u32,
    lot_id: u32,
    district_id: u32,
    building_type: u32,
    pos_x: f64,
    pos_y: f64,
    rotation: f32,
    footprint_area: f32,
    suggested_height: f32,
    estimated_cost: f32,
    generation_serial: u64,
) {
    if let Some(conn) = DB_CONN.lock().unwrap().as_ref() {
        let btype = match building_type {
            0 => BuildingTypeEnum::None,
            1 => BuildingTypeEnum::Residential,
            2 => BuildingTypeEnum::Rowhome,
            3 => BuildingTypeEnum::Retail,
            4 => BuildingTypeEnum::MixedUse,
            5 => BuildingTypeEnum::Industrial,
            6 => BuildingTypeEnum::Civic,
            7 => BuildingTypeEnum::Luxury,
            8 => BuildingTypeEnum::Utility,
            _ => BuildingTypeEnum::None,
        };
        let _ = conn.reducers.push_building(
            id, lot_id, district_id, btype,
            pos_x, pos_y, rotation, footprint_area,
            suggested_height, estimated_cost, generation_serial,
        );
    }
}

#[no_mangle]
pub extern "C" fn rc_db_push_generation_stats(
    serial: u64,
    roads: u32,
    districts: u32,
    lots: u32,
    buildings: u32,
    time_ms: f32,
) {
    if let Some(conn) = DB_CONN.lock().unwrap().as_ref() {
        let _ = conn.reducers.push_generation_stats(
            serial, roads, districts, lots, buildings, time_ms,
        );
    }
}

#[no_mangle]
pub extern "C" fn rc_db_clear_generated() {
    if let Some(conn) = DB_CONN.lock().unwrap().as_ref() {
        let _ = conn.reducers.clear_generated();
    }
}
