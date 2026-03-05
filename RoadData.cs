using System;
using System.Collections.Generic;
using System.Xml.Serialization;
using Newtonsoft.Json;
using Newtonsoft.Json.Converters;

namespace RogueOpenDRIVE.Data
{
    /// <summary>
    /// Represents the <road> element.
    /// Defined in 10.1 Introduction to roads.
    /// </summary>
    public class Road
    {
        [XmlAttribute("id")]
        [JsonProperty("id")]
        public string Id { get; set; }

        [XmlAttribute("junction")]
        [JsonProperty("junction")]
        public string Junction { get; set; } // Use "-1" for none

        [XmlAttribute("length")]
        [JsonProperty("length")]
        public double Length { get; set; }

        [XmlAttribute("name")]
        [JsonProperty("name", NullValueHandling = NullValueHandling.Ignore)]
        public string Name { get; set; }

        [XmlAttribute("rule")]
        [JsonProperty("rule")]
        public TrafficRule Rule { get; set; } = TrafficRule.RHT;

        [XmlElement("link")]
        [JsonProperty("link", NullValueHandling = NullValueHandling.Ignore)]
        public RoadLink Link { get; set; }

        [XmlElement("type")]
        [JsonProperty("type")]
        public List<RoadType> Types { get; set; } = new List<RoadType>();
        public bool ShouldSerializeTypes() => Types != null && Types.Count > 0;

        [XmlElement("elevationProfile")]
        [JsonProperty("elevationProfile", NullValueHandling = NullValueHandling.Ignore)]
        public ElevationProfile ElevationProfile { get; set; }

        [XmlElement("lateralProfile")]
        [JsonProperty("lateralProfile", NullValueHandling = NullValueHandling.Ignore)]
        public LateralProfile LateralProfile { get; set; }

        [XmlElement("surface")]
        [JsonProperty("surface", NullValueHandling = NullValueHandling.Ignore)]
        public RoadSurface Surface { get; set; }

        [XmlElement("lanes")]
        [JsonProperty("lanes", NullValueHandling = NullValueHandling.Ignore)]
        public Lanes Lanes { get; set; }
    }

    [JsonConverter(typeof(StringEnumConverter))]
    public enum TrafficRule
    {
        RHT, // Right-hand traffic
        LHT  // Left-hand traffic
    }

    /// <summary>
    /// Represents the <link> element within a road.
    /// Defined in 10.3 Road linkage.
    /// </summary>
    public class RoadLink
    {
        [XmlElement("predecessor")]
        [JsonProperty("predecessor", NullValueHandling = NullValueHandling.Ignore)]
        public RoadLinkElement Predecessor { get; set; }

        [XmlElement("successor")]
        [JsonProperty("successor", NullValueHandling = NullValueHandling.Ignore)]
        public RoadLinkElement Successor { get; set; }
    }

    public class RoadLinkElement
    {
        [XmlAttribute("elementType")]
        [JsonProperty("elementType")]
        public RoadLinkElementType ElementType { get; set; }

        [XmlAttribute("elementId")]
        [JsonProperty("elementId")]
        public string ElementId { get; set; }

        [XmlAttribute("contactPoint")]
        [JsonProperty("contactPoint", NullValueHandling = NullValueHandling.Ignore)]
        public ContactPoint? ContactPoint { get; set; }

        [XmlAttribute("elementDir")]
        [JsonProperty("elementDir", NullValueHandling = NullValueHandling.Ignore)]
        public ElementDir? ElementDir { get; set; }

        [XmlAttribute("elementS")]
        [JsonProperty("elementS", NullValueHandling = NullValueHandling.Ignore)]
        public double? ElementS { get; set; }
    }

    [JsonConverter(typeof(StringEnumConverter))]
    public enum RoadLinkElementType
    {
        road,
        junction
    }

    [JsonConverter(typeof(StringEnumConverter))]
    public enum ContactPoint
    {
        start,
        end
    }

    [JsonConverter(typeof(StringEnumConverter))]
    public enum ElementDir
    {
        same,
        opposite
    }

    /// <summary>
    /// Represents the <type> element.
    /// Defined in 10.4 Road type.
    /// </summary>
    public class RoadType
    {
        [XmlAttribute("s")]
        [JsonProperty("s")]
        public double S { get; set; }

        [XmlAttribute("type")]
        [JsonProperty("type")]
        public string Type { get; set; } 

        [XmlAttribute("country")]
        [JsonProperty("country", NullValueHandling = NullValueHandling.Ignore)]
        public string Country { get; set; } // ISO 3166-1 alpha-2

        [XmlElement("speed")]
        [JsonProperty("speed", NullValueHandling = NullValueHandling.Ignore)]
        public RoadSpeed Speed { get; set; }
    }

    public class RoadSpeed
    {
        [XmlAttribute("max")]
        [JsonProperty("max")]
        public string Max { get; set; } // "no limit", "undefined" or numerical value

        [XmlAttribute("unit")]
        [JsonProperty("unit")]
        public string Unit { get; set; } 
    }

    /// <summary>
    /// Represents the <elevationProfile> element.
    /// Defined in 10.5 Road elevation methods.
    /// </summary>
    public class ElevationProfile
    {
        [XmlElement("elevation")]
        [JsonProperty("elevation")]
        public List<Elevation> Elevations { get; set; } = new List<Elevation>();
        public bool ShouldSerializeElevations() => Elevations != null && Elevations.Count > 0;
    }

    public class Polynomial
    {
        [XmlAttribute("a")]
        [JsonProperty("a")]
        public double A { get; set; }

        [XmlAttribute("b")]
        [JsonProperty("b")]
        public double B { get; set; }

        [XmlAttribute("c")]
        [JsonProperty("c")]
        public double C { get; set; }

        [XmlAttribute("d")]
        [JsonProperty("d")]
        public double D { get; set; }
    }

    public class CubicPolynomial : Polynomial
    {
        [XmlAttribute("s")]
        [JsonProperty("s")]
        public double S { get; set; }
    }

    public class Elevation : CubicPolynomial { }

    /// <summary>
    /// Represents the <lateralProfile> element.
    /// Defined in 10.5 Road elevation methods.
    /// </summary>
    public class LateralProfile
    {
        [XmlElement("superelevation")]
        [JsonProperty("superelevation")]
        public List<Superelevation> Superelevations { get; set; } = new List<Superelevation>();
        public bool ShouldSerializeSuperelevations() => Superelevations != null && Superelevations.Count > 0;

        [XmlElement("shape")]
        [JsonProperty("shape")]
        public List<Shape> Shapes { get; set; } = new List<Shape>();
        public bool ShouldSerializeShapes() => Shapes != null && Shapes.Count > 0;

        [XmlElement("crossSectionSurface")]
        [JsonProperty("crossSectionSurface", NullValueHandling = NullValueHandling.Ignore)]
        public CrossSectionSurface CrossSectionSurface { get; set; }
    }

    public class Superelevation : CubicPolynomial { }

    public class Shape : CubicPolynomial
    {
        [XmlAttribute("t")]
        [JsonProperty("t")]
        public double T { get; set; }
    }

    public class CrossSectionSurface
    {
        [XmlElement("tOffset")]
        [JsonProperty("tOffset", NullValueHandling = NullValueHandling.Ignore)]
        public CrossSectionOffset TOffset { get; set; }

        [XmlElement("surfaceStrips")]
        [JsonProperty("surfaceStrips", NullValueHandling = NullValueHandling.Ignore)]
        public SurfaceStrips SurfaceStrips { get; set; }
    }

    public class CrossSectionOffset
    {
        [XmlElement("coefficients")]
        [JsonProperty("coefficients")]
        public List<Coefficients> Coefficients { get; set; } = new List<Coefficients>();
        public bool ShouldSerializeCoefficients() => Coefficients != null && Coefficients.Count > 0;
    }

    public class SurfaceStrips
    {
        [XmlElement("strip")]
        [JsonProperty("strip")]
        public List<Strip> Strips { get; set; } = new List<Strip>();
        public bool ShouldSerializeStrips() => Strips != null && Strips.Count > 0;
    }

    public class Strip
    {
        [XmlAttribute("id")]
        [JsonProperty("id")]
        public int Id { get; set; }

        [XmlAttribute("mode")]
        [JsonProperty("mode", NullValueHandling = NullValueHandling.Ignore)]
        public string Mode { get; set; } // "independent" or "relative"

        [XmlElement("width")]
        [JsonProperty("width", NullValueHandling = NullValueHandling.Ignore)]
        public StripPolynomial Width { get; set; }

        [XmlElement("constant")]
        [JsonProperty("constant", NullValueHandling = NullValueHandling.Ignore)]
        public StripPolynomial Constant { get; set; }

        [XmlElement("linear")]
        [JsonProperty("linear", NullValueHandling = NullValueHandling.Ignore)]
        public StripPolynomial Linear { get; set; }

        [XmlElement("quadratic")]
        [JsonProperty("quadratic", NullValueHandling = NullValueHandling.Ignore)]
        public StripPolynomial Quadratic { get; set; }

        [XmlElement("cubic")]
        [JsonProperty("cubic", NullValueHandling = NullValueHandling.Ignore)]
        public StripPolynomial Cubic { get; set; }
    }

    public class StripPolynomial
    {
        [XmlElement("coefficients")]
        [JsonProperty("coefficients")]
        public List<Coefficients> Coefficients { get; set; } = new List<Coefficients>();
        public bool ShouldSerializeCoefficients() => Coefficients != null && Coefficients.Count > 0;
    }

    public class Coefficients : CubicPolynomial { }

    /// <summary>
    /// Represents the <surface> element for CRG data.
    /// Defined in 10.6 Road CRG surface.
    /// </summary>
    public class RoadSurface
    {
        [XmlElement("CRG")]
        [JsonProperty("CRG")]
        public List<CRG> CRGs { get; set; } = new List<CRG>();
        public bool ShouldSerializeCRGs() => CRGs != null && CRGs.Count > 0;
    }

    public class CRG
    {
        [XmlAttribute("file")]
        [JsonProperty("file")]
        public string File { get; set; }

        [XmlAttribute("sStart")]
        [JsonProperty("sStart")]
        public double SStart { get; set; }

        [XmlAttribute("sEnd")]
        [JsonProperty("sEnd")]
        public double SEnd { get; set; }

        [XmlAttribute("orientation")]
        [JsonProperty("orientation")]
        public string Orientation { get; set; } // "same" or "opposite"

        [XmlAttribute("mode")]
        [JsonProperty("mode")]
        public string Mode { get; set; } // "attached", "attached0", "genuine", "global"

        [XmlAttribute("purpose")]
        [JsonProperty("purpose", NullValueHandling = NullValueHandling.Ignore)]
        public string Purpose { get; set; } // "elevation", "friction"

        [XmlAttribute("sOffset")]
        [JsonProperty("sOffset")]
        [JsonProperty("sOffset", DefaultValueHandling = DefaultValueHandling.Ignore)]
        public double SOffset { get; set; }

        [XmlAttribute("tOffset")]
        [JsonProperty("tOffset")]
        [JsonProperty("tOffset", DefaultValueHandling = DefaultValueHandling.Ignore)]
        public double TOffset { get; set; }

        [XmlAttribute("hOffset")]
        [JsonProperty("hOffset")]
        [JsonProperty("hOffset", DefaultValueHandling = DefaultValueHandling.Ignore)]
        public double HOffset { get; set; }

        [XmlAttribute("zOffset")]
        [JsonProperty("zOffset")]
        [JsonProperty("zOffset", DefaultValueHandling = DefaultValueHandling.Ignore)]
        public double ZOffset { get; set; }

        [XmlAttribute("zScale")]
        [JsonProperty("zScale")]
        [JsonProperty("zScale", DefaultValueHandling = DefaultValueHandling.Ignore)]
        public double ZScale { get; set; } = 1.0;
    }

    // =================================================================
    // Lane Data Structures
    // Based on OpenDRIVE Specification Chapter 11: Lanes
    // =================================================================

    [JsonConverter(typeof(StringEnumConverter))]
    public enum LaneType
    {
        shoulder, border, driving, stop, restricted, parking, median, biking,
        walking, curb, entry, exit, onRamp, offRamp, connectingRamp, slipLane, none,
        sidewalk, // Deprecated
        bidirectional // Deprecated
    }

    [JsonConverter(typeof(StringEnumConverter))]
    public enum LaneDirection
    {
        standard,
        reversed,
        both
    }

    [JsonConverter(typeof(StringEnumConverter))]
    public enum LaneAccessRule
    {
        allow,
        deny
    }

    [JsonConverter(typeof(StringEnumConverter))]
    public enum AccessRestrictionType
    {
        simulator, autonomousTraffic, pedestrian, passengerCar, bus, truck,
        bicycle, motorcycle, trailer, none
    }

    /// <summary>
    /// Represents the <lanes> element.
    /// Contains all lane information for a road.
    /// </summary>
    public class Lanes
    {
        [XmlElement("laneOffset")]
        [JsonProperty("laneOffset", NullValueHandling = NullValueHandling.Ignore)]
        public List<LaneOffset> LaneOffsets { get; set; } = new List<LaneOffset>();
        public bool ShouldSerializeLaneOffsets() => LaneOffsets != null && LaneOffsets.Count > 0;

        [XmlElement("laneSection")]
        [JsonProperty("laneSection")]
        public List<LaneSection> LaneSections { get; set; } = new List<LaneSection>();
        public bool ShouldSerializeLaneSections() => LaneSections != null && LaneSections.Count > 0;
    }

    /// <summary>
    /// Represents a <laneOffset> record.
    /// Defines a lateral shift of the lane reference line.
    /// </summary>
    public class LaneOffset : CubicPolynomial { }

    /// <summary>
    /// Represents a <laneSection> element.
    /// A road is split into one or more lane sections.
    /// </summary>
    public class LaneSection
    {
        [XmlAttribute("s")]
        [JsonProperty("s")]
        public double S { get; set; }

        [XmlAttribute("singleSide")]
        [JsonProperty("singleSide", DefaultValueHandling = DefaultValueHandling.Ignore)]
        public bool SingleSide { get; set; }

        [XmlElement("left")]
        [JsonProperty("left", NullValueHandling = NullValueHandling.Ignore)]
        public LaneGroupLeft Left { get; set; }

        [XmlElement("center")]
        [JsonProperty("center")]
        public LaneGroupCenter Center { get; set; }

        [XmlElement("right")]
        [JsonProperty("right", NullValueHandling = NullValueHandling.Ignore)]
        public LaneGroupRight Right { get; set; }
    }

    /// <summary>
    /// Represents the <left> lane group.
    /// </summary>
    public class LaneGroupLeft
    {
        [XmlElement("lane")]
        [JsonProperty("lane")]
        public List<Lane> Lanes { get; set; } = new List<Lane>();
        public bool ShouldSerializeLanes() => Lanes != null && Lanes.Count > 0;
    }

    /// <summary>
    /// Represents the <right> lane group.
    /// </summary>
    public class LaneGroupRight
    {
        [XmlElement("lane")]
        [JsonProperty("lane")]
        public List<Lane> Lanes { get; set; } = new List<Lane>();
        public bool ShouldSerializeLanes() => Lanes != null && Lanes.Count > 0;
    }

    /// <summary>
    /// Represents the <center> lane group.
    /// </summary>
    public class LaneGroupCenter
    {
        [XmlElement("lane")]
        [JsonProperty("lane")]
        public CenterLane Lane { get; set; }
    }

    /// <summary>
    /// Represents the center <lane> element (ID=0).
    /// </summary>
    public class CenterLane
    {
        [XmlAttribute("id")]
        [JsonProperty("id")]
        public int Id { get; set; } // Always 0

        [XmlAttribute("type")]
        [JsonProperty("type")]
        public LaneType Type { get; set; }

        [XmlAttribute("level")]
        [JsonProperty("level", DefaultValueHandling = DefaultValueHandling.Ignore)]
        public bool Level { get; set; }

        [XmlElement("link")]
        [JsonProperty("link", NullValueHandling = NullValueHandling.Ignore)]
        public LaneLink Link { get; set; }
    }

    /// <summary>
    /// Represents a <lane> element for the left or right group.
    /// </summary>
    public class Lane
    {
        [XmlAttribute("id")]
        [JsonProperty("id")]
        public int Id { get; set; }

        [XmlAttribute("type")]
        [JsonProperty("type")]
        public LaneType Type { get; set; }

        [XmlAttribute("level")]
        [JsonProperty("level", DefaultValueHandling = DefaultValueHandling.Ignore)]
        public bool Level { get; set; }

        [XmlElement("link")]
        [JsonProperty("link", NullValueHandling = NullValueHandling.Ignore)]
        public LaneLink Link { get; set; }

        [XmlElement("width")]
        [JsonProperty("width", NullValueHandling = NullValueHandling.Ignore)]
        public List<LaneWidth> Widths { get; set; } = new List<LaneWidth>();
        public bool ShouldSerializeWidths() => Widths != null && Widths.Count > 0;

        [XmlElement("border")]
        [JsonProperty("border", NullValueHandling = NullValueHandling.Ignore)]
        public List<LaneBorder> Borders { get; set; } = new List<LaneBorder>();
        public bool ShouldSerializeBorders() => Borders != null && Borders.Count > 0;

        [XmlElement("height")]
        [JsonProperty("height", NullValueHandling = NullValueHandling.Ignore)]
        public List<LaneHeight> Heights { get; set; } = new List<LaneHeight>();
        public bool ShouldSerializeHeights() => Heights != null && Heights.Count > 0;

        [XmlElement("material")]
        [JsonProperty("material", NullValueHandling = NullValueHandling.Ignore)]
        public List<LaneMaterial> Materials { get; set; } = new List<LaneMaterial>();
        public bool ShouldSerializeMaterials() => Materials != null && Materials.Count > 0;

        [XmlElement("speed")]
        [JsonProperty("speed", NullValueHandling = NullValueHandling.Ignore)]
        public List<LaneSpeed> Speeds { get; set; } = new List<LaneSpeed>();
        public bool ShouldSerializeSpeeds() => Speeds != null && Speeds.Count > 0;

        [XmlElement("access")]
        [JsonProperty("access", NullValueHandling = NullValueHandling.Ignore)]
        public List<LaneAccess> Accesses { get; set; } = new List<LaneAccess>();
        public bool ShouldSerializeAccesses() => Accesses != null && Accesses.Count > 0;

        [XmlElement("rule")]
        [JsonProperty("rule", NullValueHandling = NullValueHandling.Ignore)]
        public List<LaneRule> Rules { get; set; } = new List<LaneRule>();
        public bool ShouldSerializeRules() => Rules != null && Rules.Count > 0;

        [XmlAttribute("roadWorks")]
        [JsonProperty("roadWorks", DefaultValueHandling = DefaultValueHandling.Ignore)]
        public bool RoadWorks { get; set; }

        [XmlAttribute("direction")]
        [JsonProperty("direction", NullValueHandling = NullValueHandling.Ignore)]
        public LaneDirection? Direction { get; set; }

        [XmlAttribute("dynamicLaneDirection")]
        [JsonProperty("dynamicLaneDirection", DefaultValueHandling = DefaultValueHandling.Ignore)]
        public bool DynamicLaneDirection { get; set; }

        [XmlAttribute("dynamicLaneType")]
        [JsonProperty("dynamicLaneType", DefaultValueHandling = DefaultValueHandling.Ignore)]
        public bool DynamicLaneType { get; set; }
    }

    /// <summary>
    /// Represents a <link> element within a lane, for lane-to-lane connections.
    /// </summary>
    public class LaneLink
    {
        [XmlElement("predecessor")]
        [JsonProperty("predecessor", NullValueHandling = NullValueHandling.Ignore)]
        public List<LaneLinkPredecessorSuccessor> Predecessors { get; set; } = new List<LaneLinkPredecessorSuccessor>();
        public bool ShouldSerializePredecessors() => Predecessors != null && Predecessors.Count > 0;

        [XmlElement("successor")]
        [JsonProperty("successor", NullValueHandling = NullValueHandling.Ignore)]
        public List<LaneLinkPredecessorSuccessor> Successors { get; set; } = new List<LaneLinkPredecessorSuccessor>();
        public bool ShouldSerializeSuccessors() => Successors != null && Successors.Count > 0;
    }

    /// <summary>
    /// Represents a <predecessor> or <successor> link for a lane.
    /// </summary>
    public class LaneLinkPredecessorSuccessor
    {
        [XmlAttribute("id")]
        [JsonProperty("id")]
        public int Id { get; set; }
    }

    /// <summary>
    /// Base class for lane geometry polynomials like width and border.
    /// </summary>
    public class LanePolynomial : Polynomial
    {
        [XmlAttribute("sOffset")]
        [JsonProperty("sOffset")]
        public double SOffset { get; set; }
    }

    /// <summary>
    /// Represents a <width> record for a lane.
    /// </summary>
    public class LaneWidth : LanePolynomial { }

    /// <summary>
    /// Represents a <border> record for a lane.
    /// </summary>
    public class LaneBorder : LanePolynomial { }

    /// <summary>
    /// Represents a <height> record for a lane.
    /// </summary>
    public class LaneHeight
    {
        [XmlAttribute("sOffset")]
        [JsonProperty("sOffset")]
        public double SOffset { get; set; }

        [XmlAttribute("inner")]
        [JsonProperty("inner")]
        public double Inner { get; set; }

        [XmlAttribute("outer")]
        [JsonProperty("outer")]
        public double Outer { get; set; }
    }

    /// <summary>
    /// Represents a <material> record for a lane.
    /// </summary>
    public class LaneMaterial
    {
        [XmlAttribute("sOffset")]
        [JsonProperty("sOffset")]
        public double SOffset { get; set; }

        [XmlAttribute("surface")]
        [JsonProperty("surface", NullValueHandling = NullValueHandling.Ignore)]
        public string Surface { get; set; }

        [XmlAttribute("friction")]
        [JsonProperty("friction")]
        public double Friction { get; set; }

        [XmlAttribute("roughness")]
        [JsonProperty("roughness", DefaultValueHandling = DefaultValueHandling.Ignore)]
        public double Roughness { get; set; }
    }

    /// <summary>
    /// Represents a <speed> record for a lane.
    /// </summary>
    public class LaneSpeed
    {
        [XmlAttribute("sOffset")]
        [JsonProperty("sOffset")]
        public double SOffset { get; set; }

        [XmlAttribute("max")]
        [JsonProperty("max")]
        public double Max { get; set; }

        [XmlAttribute("unit")]
        [JsonProperty("unit", NullValueHandling = NullValueHandling.Ignore)]
        public string Unit { get; set; } // e_unitSpeed
    }

    /// <summary>
    /// Represents an <access> record for a lane.
    /// </summary>
    public class LaneAccess
    {
        [XmlAttribute("sOffset")]
        [JsonProperty("sOffset")]
        public double SOffset { get; set; }

        [XmlAttribute("rule")]
        [JsonProperty("rule")]
        public LaneAccessRule Rule { get; set; }

        [XmlElement("restriction")]
        [JsonProperty("restriction")]
        public List<LaneAccessRestriction> Restrictions { get; set; } = new List<LaneAccessRestriction>();
        public bool ShouldSerializeRestrictions() => Restrictions != null && Restrictions.Count > 0;
    }

    /// <summary>
    /// Represents a <restriction> for lane access.
    /// </summary>
    public class LaneAccessRestriction
    {
        [XmlAttribute("type")]
        [JsonProperty("type")]
        public AccessRestrictionType Type { get; set; }
    }

    /// <summary>
    /// Represents a <rule> record for a lane.
    /// </summary>
    public class LaneRule
    {
        [XmlAttribute("sOffset")]
        [JsonProperty("sOffset")]
        public double SOffset { get; set; }

        [XmlAttribute("value")]
        [JsonProperty("value")]
        public string Value { get; set; }
    }
}
