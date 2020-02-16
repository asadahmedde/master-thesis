//
// ConfigParser.hpp
// Parses JSON config for the server settings
//

#include "config/ConfigParser.hpp"
#include "nlohmann/json.hpp"

#include <iostream>
#include <boost/filesystem.hpp>

namespace Config
{
#define CONFIG_DEFAULT_PATH "../../../cv_reconstruct/resources/config/config_default.json"
#define CONFIG_FILE_PATH "config.json"

    // Parse config
    Config ConfigParser::ParseConfig()
    {
        // check if config exists
        if (!boost::filesystem::exists(CONFIG_FILE_PATH)) {
            boost::filesystem::copy_file(CONFIG_DEFAULT_PATH, CONFIG_FILE_PATH);
        }

        std::ifstream fs(CONFIG_FILE_PATH, std::ios::in);
        nlohmann::json json;
        fs >> json;
        fs.close();

        // extract json into config record
        Config config;
        nlohmann::json serverConfig = json["config"]["server"];

        // server config
        config.Server.ServerPort = serverConfig["port"];

        // reconstruction config
        nlohmann::json reconstructionConfig = json["config"]["reconstruction"];
        config.Reconstruction.ShouldRectifyImages = reconstructionConfig["requires_rectification"];

        std::string bmTypeString = reconstructionConfig["block_matcher"];
        if (bmTypeString == "stereo_bm") {
            config.Reconstruction.BlockMatcherType = Reconstruct::StereoBlockMatcherType::STEREO_BLOCK_MATCHER;
        }
        else if (bmTypeString == "stereo_sgbm") {
            config.Reconstruction.BlockMatcherType = Reconstruct::StereoBlockMatcherType::STEREO_SEMI_GLOBAL_BLOCK_MATCHER;
        }
        else {
            config.Reconstruction.BlockMatcherType = Reconstruct::StereoBlockMatcherType::STEREO_BLOCK_MATCHER;
        }

        config.Reconstruction.WindowSize = reconstructionConfig["window_size"];
        config.Reconstruction.NumDisparities = reconstructionConfig["num_disparities"];

        // point cloud post processing config
        nlohmann::json pointCloudPostProcessConfig = json["config"]["point_cloud_post_processing"];
        config.PointCloudPostProcess.OutlierMinK = pointCloudPostProcessConfig["outlier_min_k"];
        config.PointCloudPostProcess.OutlierStdDevThreshold = pointCloudPostProcessConfig["outlier_std_threshold"];

        return config;
    }
}