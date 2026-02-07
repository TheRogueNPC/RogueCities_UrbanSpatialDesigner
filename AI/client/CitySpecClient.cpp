#include "CitySpecClient.h"
#include "config/AiConfig.h"
#include "tools/HttpClient.h"
#include <iostream>

using json = nlohmann::json;

namespace RogueCity::AI {

nlohmann::json CitySpecClient::ToJson(const Core::CitySpec& spec) {
    json j;
    
    j["intent"]["description"] = spec.intent.description;
    j["intent"]["scale"] = spec.intent.scale;
    j["intent"]["climate"] = spec.intent.climate;
    j["intent"]["style_tags"] = spec.intent.styleTags;
    
    json districtsArray = json::array();
    for (const auto& d : spec.districts) {
        json dist;
        dist["type"] = d.type;
        dist["density"] = d.density;
        districtsArray.push_back(dist);
    }
    j["districts"] = districtsArray;
    
    j["seed"] = spec.seed;
    j["road_density"] = spec.roadDensity;
    
    return j;
}

Core::CitySpec CitySpecClient::FromJson(const nlohmann::json& j) {
    Core::CitySpec spec;
    
    if (j.contains("intent")) {
        spec.intent.description = j["intent"].value("description", "");
        spec.intent.scale = j["intent"].value("scale", "town");
        spec.intent.climate = j["intent"].value("climate", "");
        if (j["intent"].contains("style_tags")) {
            for (const auto& tag : j["intent"]["style_tags"]) {
                spec.intent.styleTags.push_back(tag.get<std::string>());
            }
        }
    }
    
    if (j.contains("districts")) {
        for (const auto& d : j["districts"]) {
            Core::DistrictHint hint;
            hint.type = d.value("type", "");
            hint.density = d.value("density", 0.5f);
            spec.districts.push_back(hint);
        }
    }
    
    spec.seed = j.value("seed", 0);
    spec.roadDensity = j.value("road_density", 0.5f);
    
    return spec;
}

Core::CitySpec CitySpecClient::GenerateSpec(
    const std::string& description,
    const std::string& scale
) {
    auto& config = AiConfigManager::Instance().GetConfig();
    std::string url = config.bridgeBaseUrl + "/city_spec";
    
    json requestBody;
    requestBody["description"] = description;
    requestBody["constraints"]["scale"] = scale;
    requestBody["model"] = config.citySpecModel;
    
    std::cout << "[AI] Generating CitySpec..." << std::endl;
    
    std::string responseStr = HttpClient::PostJson(url, requestBody.dump());
    
    if (responseStr.empty() || responseStr == "[]") {
        std::cerr << "[AI] Empty response from CitySpec generator" << std::endl;
        return {};
    }
    
    try {
        json response = json::parse(responseStr);
        
        if (response.contains("error")) {
            std::cerr << "[AI] CitySpec error: " << response["error"] << std::endl;
            return {};
        }
        
        if (response.contains("spec")) {
            return FromJson(response["spec"]);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "[AI] Failed to parse CitySpec response: " << e.what() << std::endl;
    }
    
    return {};
}

} // namespace RogueCity::AI
