#include "world_visuals.h"
#include "misc_utils.h"
#include "map_parser.h"
#include "../cheat_manager.h"
#include "../visuals/visualizer/visualizer.h"
#include "../../sdk/offsets/offsets.h"

void cheats::misc::c_world_visuals::run(const cache::entity_t& local)
{
    if (!globals::visuals::masterswitch) return;
    if (!globals::visuals::show_visuals) return;

    auto lighting = globals::get_lighting();
    auto renderview = globals::get_renderview();
    auto workspace = globals::get_workspace();
    auto camera = globals::get_camera();


    if (lighting.address)

    {
        memory->write<bool>(renderview.address + 0x149, false);
        memory->write<bool>(renderview.address + 0x148, false);
        if (globals::visuals::fullbright && is_key_active(globals::visuals::fullbright_key, globals::visuals::fullbright_mode))
        {            memory->write<rbx::color3_t>(lighting.address + Offsets::Lighting::Ambient, { 1.f, 1.f, 1.f });
            memory->write<rbx::color3_t>(lighting.address + Offsets::Lighting::OutdoorAmbient, { 1.f, 1.f, 1.f });
            memory->write<float>(lighting.address + Offsets::Lighting::Brightness, 5.f);
        }
        if (globals::visuals::no_fog && is_key_active(globals::visuals::no_fog_key, globals::visuals::no_fog_mode))
        {            memory->write<float>(lighting.address + Offsets::Lighting::FogStart, 0.f);
            memory->write<float>(lighting.address + Offsets::Lighting::FogEnd, 999999.f);
        }

        if (globals::visuals::force_lighting)
        {            memory->write<rbx::color3_t>(lighting.address + Offsets::Lighting::Ambient, rbx::color3_t(globals::visuals::ambient_color[0], globals::visuals::ambient_color[1], globals::visuals::ambient_color[2]));
            memory->write<rbx::color3_t>(lighting.address + Offsets::Lighting::OutdoorAmbient, rbx::color3_t(globals::visuals::outdoor_ambient_color[0], globals::visuals::outdoor_ambient_color[1], globals::visuals::outdoor_ambient_color[2]));
            memory->write<rbx::color3_t>(lighting.address + Offsets::Lighting::ColorShift_Top, rbx::color3_t(globals::visuals::colorshift_top[0], globals::visuals::colorshift_top[1], globals::visuals::colorshift_top[2]));
            memory->write<rbx::color3_t>(lighting.address + Offsets::Lighting::ColorShift_Bottom, rbx::color3_t(globals::visuals::colorshift_bottom[0], globals::visuals::colorshift_bottom[1], globals::visuals::colorshift_bottom[2]));
            memory->write<float>(lighting.address + Offsets::Lighting::Brightness, globals::visuals::brightness);
            memory->write<float>(lighting.address + Offsets::Lighting::ExposureCompensation, globals::visuals::exposure);
            memory->write<rbx::color3_t>(lighting.address + Offsets::Lighting::FogColor, rbx::color3_t(globals::visuals::fog_color[0], globals::visuals::fog_color[1], globals::visuals::fog_color[2]));
            memory->write<float>(lighting.address + Offsets::Lighting::FogStart, globals::visuals::fog_start);
            memory->write<float>(lighting.address + Offsets::Lighting::FogEnd, globals::visuals::fog_end);
            memory->write<float>(lighting.address + Offsets::Lighting::GeographicLatitude, globals::visuals::geographic_latitude);
            memory->write<float>(lighting.address + Offsets::Lighting::ClockTime, globals::visuals::time_of_day);
            memory->write<rbx::color3_t>(lighting.address + Offsets::Lighting::ShadowColor, rbx::color3_t(globals::visuals::shadow_color[0], globals::visuals::shadow_color[1], globals::visuals::shadow_color[2]));
            
            memory->write<float>(lighting.address + Offsets::Lighting::EnvironmentDiffuseScale, globals::visuals::environment_diffuse_scale);
            memory->write<float>(lighting.address + Offsets::Lighting::EnvironmentSpecularScale, globals::visuals::environment_specular_scale);

            memory->write<bool>(renderview.address + 0x149, false);
            memory->write<bool>(renderview.address + 0x148, false);
    }

        if (globals::visuals::no_shadows && is_key_active(globals::visuals::no_shadows_key, globals::visuals::no_shadows_mode))
        {            memory->write<bool>(lighting.address + Offsets::Lighting::GlobalShadows, false);
        }


        if (!m_cached_sky.address || std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - m_last_lighting_check).count() > 5)
        {            m_cached_sky = lighting.find_first_child_by_class("Sky");
            m_cached_atmosphere = lighting.find_first_child_by_class("Atmosphere");
            m_cached_bloom = lighting.find_first_child_by_class("BloomEffect");
            m_cached_sunrays = lighting.find_first_child_by_class("SunRaysEffect");
            m_cached_color_correction = lighting.find_first_child_by_class("ColorCorrectionEffect");
            m_cached_depth_of_field = lighting.find_first_child_by_class("DepthOfFieldEffect");
            m_last_lighting_check = std::chrono::steady_clock::now();
        }

        if (m_cached_atmosphere.address && globals::visuals::atmosphere::enabled)
        {
            memory->write<rbx::color3_t>(m_cached_atmosphere.address + Offsets::Atmosphere::Color, rbx::color3_t(globals::visuals::atmosphere::color[0], globals::visuals::atmosphere::color[1], globals::visuals::atmosphere::color[2]));
            memory->write<rbx::color3_t>(m_cached_atmosphere.address + Offsets::Atmosphere::Decay, rbx::color3_t(globals::visuals::atmosphere::decay[0], globals::visuals::atmosphere::decay[1], globals::visuals::atmosphere::decay[2]));
            memory->write<float>(m_cached_atmosphere.address + Offsets::Atmosphere::Density, globals::visuals::atmosphere::density);
            memory->write<float>(m_cached_atmosphere.address + Offsets::Atmosphere::Glare, globals::visuals::atmosphere::glare);
            memory->write<float>(m_cached_atmosphere.address + Offsets::Atmosphere::Haze, globals::visuals::atmosphere::haze);
            memory->write<float>(m_cached_atmosphere.address + Offsets::Atmosphere::Offset, globals::visuals::atmosphere::offset);
        }

        if (m_cached_bloom.address && globals::visuals::bloom::enabled)
        {
            memory->write<float>(m_cached_bloom.address + Offsets::BloomEffect::Intensity, globals::visuals::bloom::intensity);
            memory->write<float>(m_cached_bloom.address + Offsets::BloomEffect::Size, globals::visuals::bloom::size);
            memory->write<float>(m_cached_bloom.address + Offsets::BloomEffect::Threshold, globals::visuals::bloom::threshold);
        }

        if (m_cached_sunrays.address && globals::visuals::sunrays::enabled)
        {
            memory->write<float>(m_cached_sunrays.address + Offsets::SunRaysEffect::Intensity, globals::visuals::sunrays::intensity);
            memory->write<float>(m_cached_sunrays.address + Offsets::SunRaysEffect::Spread, globals::visuals::sunrays::spread);
        }

        if (m_cached_color_correction.address && globals::visuals::color_correction::enabled)
        {
            memory->write<float>(m_cached_color_correction.address + Offsets::ColorCorrectionEffect::Brightness, globals::visuals::color_correction::brightness);
            memory->write<float>(m_cached_color_correction.address + Offsets::ColorCorrectionEffect::Contrast, globals::visuals::color_correction::contrast);
            memory->write<float>(m_cached_color_correction.address + Offsets::ColorCorrectionEffect::Saturation, globals::visuals::color_correction::saturation);
            memory->write<rbx::color3_t>(m_cached_color_correction.address + Offsets::ColorCorrectionEffect::TintColor, rbx::color3_t(globals::visuals::color_correction::tint_color[0], globals::visuals::color_correction::tint_color[1], globals::visuals::color_correction::tint_color[2]));
        }

        if (m_cached_depth_of_field.address && globals::visuals::depth_of_field::enabled)
        {
            memory->write<float>(m_cached_depth_of_field.address + Offsets::DepthOfFieldEffect::Density, globals::visuals::depth_of_field::density);
            memory->write<float>(m_cached_depth_of_field.address + Offsets::DepthOfFieldEffect::FocusDistance, globals::visuals::depth_of_field::focus_distance);
            memory->write<float>(m_cached_depth_of_field.address + Offsets::DepthOfFieldEffect::InFocusRadius, globals::visuals::depth_of_field::in_focus_radius);
            memory->write<float>(m_cached_depth_of_field.address + Offsets::DepthOfFieldEffect::NearIntensity, globals::visuals::depth_of_field::near_intensity);
        }


        if (m_cached_sky.address)
        {            if (globals::visuals::celestial_bodies)
            {
            }
        }
    }


    if (workspace.address && !globals::misc::fly_hack)
    {        memory->write<float>(workspace.address + Offsets::Workspace::Gravity, globals::visuals::world_gravity);

        if (globals::visuals::terrain::enabled)
        {
            auto terrain = workspace.find_first_child_by_class("Terrain");
            if (terrain.address)
            {
                memory->write<float>(terrain.address + Offsets::Terrain::GrassLength, globals::visuals::terrain::grass_length);
                memory->write<rbx::color3_t>(terrain.address + Offsets::Terrain::WaterColor, rbx::color3_t(globals::visuals::terrain::water_color[0], globals::visuals::terrain::water_color[1], globals::visuals::terrain::water_color[2]));
                memory->write<float>(terrain.address + Offsets::Terrain::WaterReflectance, globals::visuals::terrain::water_reflectance);
                memory->write<float>(terrain.address + Offsets::Terrain::WaterTransparency, globals::visuals::terrain::water_transparency);
                memory->write<float>(terrain.address + Offsets::Terrain::WaterWaveSize, globals::visuals::terrain::water_wave_size);
                memory->write<float>(terrain.address + Offsets::Terrain::WaterWaveSpeed, globals::visuals::terrain::water_wave_speed);
            }
        }
    }

    if (camera.address)
    {
        float target_fov = globals::visuals::fov;
        if (globals::misc::zoom_enabled && is_key_active(globals::misc::zoom_key, globals::misc::zoom_mode))
        {
            target_fov = globals::misc::zoom_value;
        }
        
        // ViewModel FOV logic - often just an override or adjustment
        if (globals::visuals::viewmodel::enabled)
        {
            target_fov = globals::visuals::viewmodel::fov;
        }

        memory->write<float>(camera.address + Offsets::Camera::FieldOfView, (target_fov * 0.0174533f));

        if (globals::visuals::viewmodel::enabled)
        {
            // In Roblox, viewmodels are often models named "ViewModel" or similar inside the camera
            auto children = camera.get_children();
            for (auto& child : children)
            {
                std::string name = child.get_name();
                if (name.find("ViewModel") != std::string::npos || name.find("View Model") != std::string::npos || name.find("Weapon") != std::string::npos)
                {
                    // If it's a model, we need to apply the offset to all its parts
                    auto parts = child.get_children();
                    for (auto& part_instance : parts)
                    {
                        rbx::part_t part(part_instance.address);
                        if (part.address)
                        {
                            rbx::vector3_t pos = part.get_part_position();
                            pos.x += globals::visuals::viewmodel::offset[0];
                            pos.y += globals::visuals::viewmodel::offset[1];
                            pos.z += globals::visuals::viewmodel::offset[2];
                            part.set_part_position(pos);
                        }
                    }
                }
            }
        }

        if (globals::visuals::third_person)

        {
            memory->write<bool>(local.instance.address + 0xd5, globals::visuals::third_person);
            memory->write<bool>(local.humanoid.address + 0xd5, globals::visuals::third_person);
        memory->write<float>(camera.address + Offsets::Player::MinZoomDistance, globals::visuals::min_zoom);
            memory->write<float>(camera.address + Offsets::Player::MaxZoomDistance, globals::visuals::max_zoom);
        }


        if (globals::misc::camera_offset_enabled && local.instance.address)
        {            auto character = memory->read<std::uintptr_t>(local.instance.address + Offsets::Player::ModelInstance);
            if (character)
            {                auto humanoid = rbx::instance_t(character).find_first_child_by_class("Humanoid");
                if (humanoid.address)
                {                    memory->write<rbx::vector3_t>(humanoid.address + Offsets::Humanoid::CameraOffset,
                        rbx::vector3_t(globals::misc::camera_offset[0], globals::misc::camera_offset[1], globals::misc::camera_offset[2]));
                }
            }
        }
    }


    if (globals::visuals::map_debug)
    {
        auto& parser = c_map_parser::get();
        std::shared_lock lock(parser.get_mutex());
        const auto& parts = parser.get_cached_parts();

        rbx::vector2_t dims = globals::visualengine.get_dimensions();
        rbx::matrix4_t view = globals::visualengine.get_viewmatrix();
        ImVec2 window_offset = visualizer::get_window_offset();

        for (const auto& part : parts)
        {
            visualizer::draw_obb(part.position, part.size, part.rotation, IM_COL32(255, 255, 255, 100), dims, view, window_offset);
        }
    }
}


void cheats::misc::world_visuals()
{
    static std::unique_ptr<c_world_visuals> instance = std::make_unique<c_world_visuals>();
    cache::entity_t dummy{};
    instance->run(dummy);
}

