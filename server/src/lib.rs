use spacetimedb::{ReducerContext, Table, Timestamp, Identity};

// ====================================================================
// SHARED TYPES
// ====================================================================

#[derive(spacetimedb::SpacetimeType, Clone, Debug, PartialEq)]
pub struct Vec2 {
    pub x: f64,
    pub y: f64,
}

#[derive(spacetimedb::SpacetimeType, Clone, Debug, PartialEq)]
pub enum AxiomType {
    Organic,
    Grid,
    Radial,
    Hexagonal,
    Stem,
    LooseGrid,
    Suburban,
    Superblock,
    Linear,
    GridCorrective,
}

#[derive(spacetimedb::SpacetimeType, Clone, Debug, PartialEq)]
pub enum RoadType {
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
    MMajor,
    MMinor,
}

#[derive(spacetimedb::SpacetimeType, Clone, Debug, PartialEq)]
pub enum DistrictType {
    Mixed,
    Residential,
    Commercial,
    Civic,
    Industrial,
}

#[derive(spacetimedb::SpacetimeType, Clone, Debug, PartialEq)]
pub enum LotType {
    None,
    Residential,
    RowhomeCompact,
    RetailStrip,
    MixedUse,
    LogisticsIndustrial,
    CivicCultural,
    LuxuryScenic,
    BufferStrip,
}

#[derive(spacetimedb::SpacetimeType, Clone, Debug, PartialEq)]
pub enum BuildingTypeEnum {
    None,
    Residential,
    Rowhome,
    Retail,
    MixedUse,
    Industrial,
    Civic,
    Luxury,
    Utility,
}

// ====================================================================
// TABLES
// ====================================================================

#[spacetimedb::table(accessor = axiom, public)]
#[derive(Clone)]
pub struct Axiom {
    #[primary_key]
    #[auto_inc]
    pub id: u32,
    pub axiom_type: AxiomType,
    pub position: Vec2,
    pub radius: f64,
    pub theta: f64,
    pub decay: f64,
    pub is_user_placed: bool,
    pub main_road_points: Vec<Vec2>,
}

#[spacetimedb::table(accessor = road, public)]
#[derive(Clone)]
pub struct Road {
    #[primary_key]
    pub id: u32,
    pub road_type: RoadType,
    pub source_axiom_id: i32,
    pub points: Vec<Vec2>,
    pub layer_id: i32,
    pub has_grade_separation: bool,
    pub contains_signal: bool,
    pub contains_crosswalk: bool,
    pub generation_serial: u64,
}

#[spacetimedb::table(accessor = district, public)]
#[derive(Clone)]
pub struct District {
    #[primary_key]
    pub id: u32,
    pub district_type: DistrictType,
    pub primary_axiom_id: i32,
    pub border: Vec<Vec2>,
    pub budget_allocated: f32,
    pub projected_population: u32,
    pub desirability: f32,
    pub average_elevation: f32,
    pub generation_serial: u64,
}

#[spacetimedb::table(accessor = lot, public)]
#[derive(Clone)]
pub struct Lot {
    #[primary_key]
    pub id: u32,
    pub district_id: u32,
    pub lot_type: LotType,
    pub centroid: Vec2,
    pub boundary: Vec<Vec2>,
    pub area: f32,
    pub access: f32,
    pub exposure: f32,
    pub serviceability: f32,
    pub privacy: f32,
    pub generation_serial: u64,
}

#[spacetimedb::table(accessor = building, public)]
#[derive(Clone)]
pub struct Building {
    #[primary_key]
    pub id: u32,
    pub lot_id: u32,
    pub district_id: u32,
    pub building_type: BuildingTypeEnum,
    pub position: Vec2,
    pub rotation_radians: f32,
    pub footprint_area: f32,
    pub suggested_height: f32,
    pub estimated_cost: f32,
    pub generation_serial: u64,
}

#[spacetimedb::table(accessor = generation_intent, public)]
#[derive(Clone)]
pub struct GenerationIntent {
    #[primary_key]
    #[auto_inc]
    pub intent_id: u64,
    pub timestamp: Timestamp,
    pub client_id: Identity,
    pub description: String,
}

/// Generation stats snapshot — one row per completed generation pass
#[spacetimedb::table(accessor = generation_stats, public)]
#[derive(Clone)]
pub struct GenerationStats {
    #[primary_key]
    #[auto_inc]
    pub stats_id: u64,
    pub serial: u64,
    pub roads_generated: u32,
    pub districts_generated: u32,
    pub lots_generated: u32,
    pub buildings_generated: u32,
    pub generation_time_ms: f32,
    pub timestamp: Timestamp,
}

// ====================================================================
// REDUCERS
// ====================================================================

#[spacetimedb::reducer(init)]
pub fn init(_ctx: &ReducerContext) {
    log::info!("RogueCities Database Initialized");
}

// --- Axiom Reducers ---

#[spacetimedb::reducer]
pub fn place_axiom(
    ctx: &ReducerContext,
    axiom_type: AxiomType,
    pos_x: f64,
    pos_y: f64,
    radius: f64,
    theta: f64,
    decay: f64,
) -> Result<(), String> {
    let new_axiom = Axiom {
        id: 0,
        axiom_type,
        position: Vec2 { x: pos_x, y: pos_y },
        radius,
        theta,
        decay,
        is_user_placed: true,
        main_road_points: Vec::new(),
    };
    ctx.db.axiom().insert(new_axiom);

    ctx.db.generation_intent().insert(GenerationIntent {
        intent_id: 0,
        timestamp: ctx.timestamp,
        client_id: ctx.sender(),
        description: "Axiom Placed".to_string(),
    });

    Ok(())
}

#[spacetimedb::reducer]
pub fn clear_axioms(ctx: &ReducerContext) -> Result<(), String> {
    for axiom in ctx.db.axiom().iter() {
        ctx.db.axiom().delete(axiom);
    }

    ctx.db.generation_intent().insert(GenerationIntent {
        intent_id: 0,
        timestamp: ctx.timestamp,
        client_id: ctx.sender(),
        description: "Axioms Cleared".to_string(),
    });

    Ok(())
}

// --- Bulk Push Reducers (called by Headless Compute Worker) ---

#[spacetimedb::reducer]
pub fn push_road(
    ctx: &ReducerContext,
    id: u32,
    road_type: RoadType,
    source_axiom_id: i32,
    points: Vec<Vec2>,
    layer_id: i32,
    has_grade_separation: bool,
    contains_signal: bool,
    contains_crosswalk: bool,
    generation_serial: u64,
) -> Result<(), String> {
    // Upsert: delete existing if present, then insert
    if let Some(existing) = ctx.db.road().id().find(id) {
        ctx.db.road().delete(existing);
    }
    ctx.db.road().insert(Road {
        id,
        road_type,
        source_axiom_id,
        points,
        layer_id,
        has_grade_separation,
        contains_signal,
        contains_crosswalk,
        generation_serial,
    });
    Ok(())
}

#[spacetimedb::reducer]
pub fn push_district(
    ctx: &ReducerContext,
    id: u32,
    district_type: DistrictType,
    primary_axiom_id: i32,
    border: Vec<Vec2>,
    budget_allocated: f32,
    projected_population: u32,
    desirability: f32,
    average_elevation: f32,
    generation_serial: u64,
) -> Result<(), String> {
    if let Some(existing) = ctx.db.district().id().find(id) {
        ctx.db.district().delete(existing);
    }
    ctx.db.district().insert(District {
        id,
        district_type,
        primary_axiom_id,
        border,
        budget_allocated,
        projected_population,
        desirability,
        average_elevation,
        generation_serial,
    });
    Ok(())
}

#[spacetimedb::reducer]
pub fn push_lot(
    ctx: &ReducerContext,
    id: u32,
    district_id: u32,
    lot_type: LotType,
    centroid_x: f64,
    centroid_y: f64,
    boundary: Vec<Vec2>,
    area: f32,
    access: f32,
    exposure: f32,
    serviceability: f32,
    privacy: f32,
    generation_serial: u64,
) -> Result<(), String> {
    if let Some(existing) = ctx.db.lot().id().find(id) {
        ctx.db.lot().delete(existing);
    }
    ctx.db.lot().insert(Lot {
        id,
        district_id,
        lot_type,
        centroid: Vec2 { x: centroid_x, y: centroid_y },
        boundary,
        area,
        access,
        exposure,
        serviceability,
        privacy,
        generation_serial,
    });
    Ok(())
}

#[spacetimedb::reducer]
pub fn push_building(
    ctx: &ReducerContext,
    id: u32,
    lot_id: u32,
    district_id: u32,
    building_type: BuildingTypeEnum,
    pos_x: f64,
    pos_y: f64,
    rotation_radians: f32,
    footprint_area: f32,
    suggested_height: f32,
    estimated_cost: f32,
    generation_serial: u64,
) -> Result<(), String> {
    if let Some(existing) = ctx.db.building().id().find(id) {
        ctx.db.building().delete(existing);
    }
    ctx.db.building().insert(Building {
        id,
        lot_id,
        district_id,
        building_type,
        position: Vec2 { x: pos_x, y: pos_y },
        rotation_radians,
        footprint_area,
        suggested_height,
        estimated_cost,
        generation_serial,
    });
    Ok(())
}

#[spacetimedb::reducer]
pub fn push_generation_stats(
    ctx: &ReducerContext,
    serial: u64,
    roads_generated: u32,
    districts_generated: u32,
    lots_generated: u32,
    buildings_generated: u32,
    generation_time_ms: f32,
) -> Result<(), String> {
    ctx.db.generation_stats().insert(GenerationStats {
        stats_id: 0,
        serial,
        roads_generated,
        districts_generated,
        lots_generated,
        buildings_generated,
        generation_time_ms,
        timestamp: ctx.timestamp,
    });
    Ok(())
}

/// Clear all generated entities for a fresh re-generation cycle
#[spacetimedb::reducer]
pub fn clear_generated(ctx: &ReducerContext) -> Result<(), String> {
    for row in ctx.db.road().iter() {
        ctx.db.road().delete(row);
    }
    for row in ctx.db.district().iter() {
        ctx.db.district().delete(row);
    }
    for row in ctx.db.lot().iter() {
        ctx.db.lot().delete(row);
    }
    for row in ctx.db.building().iter() {
        ctx.db.building().delete(row);
    }

    ctx.db.generation_intent().insert(GenerationIntent {
        intent_id: 0,
        timestamp: ctx.timestamp,
        client_id: ctx.sender(),
        description: "Generated Data Cleared".to_string(),
    });

    Ok(())
}
