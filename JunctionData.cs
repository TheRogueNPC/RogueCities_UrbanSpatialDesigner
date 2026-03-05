using System.Collections.Generic;
using System.Linq;
using System.Xml.Serialization;
using Newtonsoft.Json;
using Newtonsoft.Json.Converters;

namespace RogueOpenDRIVE.Data
{
    /// <summary>
    /// Represents the <junction> element. Junctions are used to connect multiple roads.
    /// Based on OpenDRIVE 1.8.1 Specification, Chapter 12.
    /// </summary>
    public class Junction
    {
        [XmlAttribute("id")]
        [JsonProperty("id")]
        public string Id { get; set; }

        [XmlAttribute("name")]
        [JsonProperty("name", NullValueHandling = NullValueHandling.Ignore)]
        public string Name { get; set; }

        [XmlAttribute("type")]
        [JsonProperty("type", DefaultValueHandling = DefaultValueHandling.Ignore)]
        public JunctionType Type { get; set; } = JunctionType.Default;

        #region Virtual Junction Attributes
        [XmlAttribute("mainRoad")]
        [JsonProperty("mainRoad", NullValueHandling = NullValueHandling.Ignore)]
        public string MainRoad { get; set; }

        [XmlAttribute("sStart")]
        [JsonProperty("sStart", NullValueHandling = NullValueHandling.Ignore)]
        public double? SStart { get; set; }

        [XmlAttribute("sEnd")]
        [JsonProperty("sEnd", NullValueHandling = NullValueHandling.Ignore)]
        public double? SEnd { get; set; }

        [XmlAttribute("orientation")]
        [JsonProperty("orientation", NullValueHandling = NullValueHandling.Ignore)]
        public string Orientation { get; set; } // e_orientation: "+", "-", "none"
        #endregion

        [XmlElement("connection")]
        [JsonProperty("connection")]
        public List<JunctionConnection> Connections { get; set; } = new List<JunctionConnection>();
        public bool ShouldSerializeConnections() => Connections != null && Connections.Count > 0;

        [XmlElement("priority")]
        [JsonProperty("priority", NullValueHandling = NullValueHandling.Ignore)]
        public List<JunctionPriority> Priorities { get; set; } = new List<JunctionPriority>();
        public bool ShouldSerializePriorities() => Priorities != null && Priorities.Count > 0;

        [XmlElement("controller")]
        [JsonProperty("controller", NullValueHandling = NullValueHandling.Ignore)]
        public List<JunctionController> Controllers { get; set; } = new List<JunctionController>();
        public bool ShouldSerializeControllers() => Controllers != null && Controllers.Count > 0;

        [XmlElement("crossPath")]
        [JsonProperty("crossPath", NullValueHandling = NullValueHandling.Ignore)]
        public List<JunctionCrossPath> CrossPaths { get; set; } = new List<JunctionCrossPath>();
        public bool ShouldSerializeCrossPaths() => CrossPaths != null && CrossPaths.Count > 0;

        /// <summary>
        /// Automagically deduces the Junction Type based on ASAM OpenDRIVE 1.8.1 rules.
        /// </summary>
        public void DeduceType()
        {
            // Rule: Virtual junctions must have a mainRoad attribute.
            if (!string.IsNullOrEmpty(MainRoad))
            {
                Type = JunctionType.Virtual;
                return;
            }

            // Rule: Direct junctions use 'linkedRoad' in connections instead of 'connectingRoad'.
            // Common junctions use 'connectingRoad'.
            if (Connections != null && Connections.Any(c => !string.IsNullOrEmpty(c.LinkedRoad)))
            {
                Type = JunctionType.Direct;
                return;
            }

            // Default fallback (Common)
            Type = JunctionType.Default;
        }

        /// <summary>
        /// Stub for custom junction generation logic.
        /// This allows for procedural generation or manual overrides of junction geometry.
        /// </summary>
        public void GenerateCustomJunction()
        {
            // TODO: Implement custom junction logic here.
            // This serves as a hook for the custom junction maker.
        }
    }

    [JsonConverter(typeof(StringEnumConverter))]
    public enum JunctionType
    {
        [XmlEnum("default")]
        Default,
        [XmlEnum("direct")]
        Direct,
        [XmlEnum("virtual")]
        Virtual,
        [XmlEnum("crossing")]
        Crossing
    }

    /// <summary>
    /// Represents a <connection> element within a junction.
    /// </summary>
    public class JunctionConnection
    {
        [XmlAttribute("id")]
        [JsonProperty("id")]
        public string Id { get; set; }

        [XmlAttribute("incomingRoad")]
        [JsonProperty("incomingRoad", NullValueHandling = NullValueHandling.Ignore)]
        public string IncomingRoad { get; set; }

        [XmlAttribute("connectingRoad")]
        [JsonProperty("connectingRoad", NullValueHandling = NullValueHandling.Ignore)]
        public string ConnectingRoad { get; set; } // For common/default junctions

        [XmlAttribute("linkedRoad")]
        [JsonProperty("linkedRoad", NullValueHandling = NullValueHandling.Ignore)]
        public string LinkedRoad { get; set; } // For direct junctions

        [XmlAttribute("contactPoint")]
        [JsonProperty("contactPoint", NullValueHandling = NullValueHandling.Ignore)]
        public ContactPoint? ContactPoint { get; set; }

        [XmlElement("laneLink")]
        [JsonProperty("laneLink")]
        public List<JunctionConnectionLaneLink> LaneLinks { get; set; } = new List<JunctionConnectionLaneLink>();
        public bool ShouldSerializeLaneLinks() => LaneLinks != null && LaneLinks.Count > 0;
    }

    /// <summary>
    /// Represents a <laneLink> within a junction connection.
    /// </summary>
    public class JunctionConnectionLaneLink
    {
        [XmlAttribute("from")]
        [JsonProperty("from")]
        public int From { get; set; }

        [XmlAttribute("to")]
        [JsonProperty("to")]
        public int To { get; set; }

        [XmlAttribute("overlapZone")]
        [JsonProperty("overlapZone", DefaultValueHandling = DefaultValueHandling.Ignore)]
        public double OverlapZone { get; set; }
    }

    /// <summary>
    /// Represents a <priority> element, defining road priority within a junction.
    /// </summary>
    public class JunctionPriority
    {
        [XmlAttribute("high")]
        [JsonProperty("high", NullValueHandling = NullValueHandling.Ignore)]
        public string High { get; set; }

        [XmlAttribute("low")]
        [JsonProperty("low", NullValueHandling = NullValueHandling.Ignore)]
        public string Low { get; set; }
    }

    /// <summary>
    /// Represents a <controller> element, referencing a signal controller.
    /// </summary>
    public class JunctionController
    {
        [XmlAttribute("id")]
        [JsonProperty("id")]
        public string Id { get; set; }

        [XmlAttribute("type")]
        [JsonProperty("type", NullValueHandling = NullValueHandling.Ignore)]
        public string Type { get; set; }

        [XmlAttribute("sequence")]
        [JsonProperty("sequence", NullValueHandling = NullValueHandling.Ignore)]
        public uint? Sequence { get; set; }
    }

    /// <summary>
    /// Represents a <crossPath> element for pedestrian/bicycle crossings.
    /// </summary>
    public class JunctionCrossPath
    {
        [XmlAttribute("id")]
        [JsonProperty("id")]
        public string Id { get; set; }

        [XmlAttribute("crossingRoad")]
        [JsonProperty("crossingRoad")]
        public string CrossingRoad { get; set; }

        [XmlAttribute("roadAtStart")]
        [JsonProperty("roadAtStart")]
        public string RoadAtStart { get; set; }

        [XmlAttribute("roadAtEnd")]
        [JsonProperty("roadAtEnd")]
        public string RoadAtEnd { get; set; }

        [XmlElement("startLaneLink")]
        [JsonProperty("startLaneLink")]
        public JunctionCrossPathLaneLink StartLaneLink { get; set; }

        [XmlElement("endLaneLink")]
        [JsonProperty("endLaneLink")]
        public JunctionCrossPathLaneLink EndLaneLink { get; set; }
    }

    /// <summary>
    /// Represents the lane links for a cross path.
    /// </summary>
    public class JunctionCrossPathLaneLink
    {
        [XmlAttribute("s")]
        [JsonProperty("s")]
        public double S { get; set; }

        [XmlAttribute("from")]
        [JsonProperty("from")]
        public int From { get; set; }

        [XmlAttribute("to")]
        [JsonProperty("to")]
        public int To { get; set; }
    }
}