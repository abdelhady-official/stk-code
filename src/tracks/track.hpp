//  SuperTuxKart - a fun racing game with go-kart
//  Copyright (C) 2004 Steve Baker <sjbaker1@airmail.net>
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 3
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#ifndef HEADER_TRACK_HPP
#define HEADER_TRACK_HPP

#ifdef __APPLE__
#  include <OpenGL/gl.h>
#else
#  ifdef WIN32
#    define WIN32_LEAN_AND_MEAN
#    include <windows.h>
#  endif
#  include <GL/gl.h>
#endif

#include <string>
#include <vector>

#include "irrlicht.h"
using namespace irr;

#include "LinearMath/btTransform.h"
#include "audio/music_information.hpp"
#include "graphics/material.hpp"
#include "items/item.hpp"
#include "tracks/quad_graph.hpp"
#include "utils/vec3.hpp"

class TriangleMesh;
class MovingTexture;
class XMLNode;
class PhysicalObject;
class BezierCurve;
class AnimationManager;
class CheckManager;

class Track
{
private:
    float                    m_gravity;
    std::string              m_ident;
    std::string              m_screenshot;
    std::vector<MusicInformation*> m_music;
    std::vector<float>       m_start_x, m_start_y, m_start_z, m_start_heading;
    std::string              m_item_style;
    std::string              m_description;
    std::string              m_designer;
    std::string              m_filename;
    std::vector<std::string> m_groups;
    std::vector<scene::ISceneNode*> m_all_nodes;
    std::vector<scene::IMesh*>      m_all_meshes;
    scene::ILightSceneNode  *m_light;
    TriangleMesh*            m_track_mesh;
    TriangleMesh*            m_non_collision_mesh;
    bool                     m_has_final_camera;
    Vec3                     m_camera_final_position;
    Vec3                     m_camera_final_hpr;
    bool                     m_is_arena;
    int                      m_version;
    bool                     loadMainTrack(const XMLNode &node);
    void                     createWater(const XMLNode &node);
    /** The graph used to connect the quads. */
    QuadGraph               *m_quad_graph;
    /** The type of sky to be used for the track. */
    enum {SKY_NONE, SKY_BOX, 
          SKY_DOME}          m_sky_type;
    /** A list of the textures for the sky to use. It contains one texture
     *  in case of a dome, and 6 textures for a box. */
    std::vector<std::string> m_sky_textures;

    /** The list of all animated textures. */
    std::vector<MovingTexture*> m_animated_textures;

    /** List of all physical objects. */
    std::vector<PhysicalObject*> m_physical_objects;

    /** If a sky dome is used, the number of horizontal segments 
     *  the sphere should be divided in. */
    int                      m_sky_hori_segments;
    /** If a sky dome is used, the number of vertical segments 
     *  the sphere should be divided in. */
    int                      m_sky_vert_segments;
    /** If a sky dome is used, percentage of the sphere to be used. */
    float                    m_sky_sphere_percent;
    /** If a sky dome is used, percentage of the texture to be used. */
    float                    m_sky_texture_percent;

    /** The texture for the mini map, which is displayed in the race gui. */
    video::ITexture         *m_mini_map;

	/** List of all bezier curves in the track - for e.g. camera, ... */
	std::vector<BezierCurve*> m_all_curves;

	/** Animation manager. */
	AnimationManager         *m_animation_manager;

	/** Checkline manager. */
	CheckManager             *m_check_manager;

	void loadCurves(const XMLNode &node);
    void handleAnimatedTextures(scene::ISceneNode *node, const XMLNode &xml);

public:

    std::string     m_name;
    bool            m_use_fog;
    float           m_fog_density;
    float           m_fog_start;
    float           m_fog_end;
    core::vector3df m_sun_position;
    video::SColorf  m_ambient_color;
    video::SColorf  m_specular_color;
    video::SColorf  m_diffuse_color;
    video::SColorf  m_sky_color;
    video::SColorf  m_fog_color;

    //FIXME: Maybe the next 4 vectors should be inside an struct and be used
    //from a vector of structs?
    //FIXME: should the driveline be set as a sgVec2?
private:
    std::vector<Vec3>    m_driveline;
	//Left and Right drivelines for overhead map rendering.
    std::vector<Vec3>    m_left_driveline;
    std::vector<Vec3>    m_right_driveline;
public:
    
    /** Start positions for arenas (unused in linear races) */
    std::vector<Vec3>   m_start_positions;
    

    Vec3 m_driveline_min;
    Vec3 m_driveline_max;


    float m_total_distance;
    static const float NOHIT;

    float m_track_2d_width,  // Width and heigth of the 2d display of the track
          m_track_2d_height;
    float m_scale_x,        // Scaling to fit track into the size determined by
          m_scale_y;        // track2dWidth/Heightheigth
    bool m_do_stretch;      // 2d track display might be stretched to fit better

                       Track             (std::string filename,float w=100,
                                          float h=100, bool stretch=1);
                      ~Track             ();
    bool               isArena           () const { return m_is_arena; }
    void               cleanup           ();
    void               draw2Dview        (float x_offset,
                                          float y_offset              ) const;
    void               drawScaled2D      (float x, float y, float w,
                                          float h                     ) const;

    const Vec3&        trackToSpatial    (const int SECTOR) const;
    void               loadTrackModel    ();
    void               addMusic          (MusicInformation* mi)
                                                  {m_music.push_back(mi);       }
    float              getGravity        () const {return m_gravity;            }
    /** Returns the version of the .track file. */
    int                getVersion        () const {return m_version;            }
    float              getTrackLength    () const {return m_total_distance;     }
    const std::string& getIdent          () const {return m_ident;              }
    const char*        getName           () const {return m_name.c_str();       }
    const std::vector<std::string>
                       getGroups         () const {return m_groups;             }
    void               startMusic        () const;
    const std::string& getFilename       () const {return m_filename;           }
    const core::vector3df& getSunPos     () const {return m_sun_position;       }
    const video::SColorf& getAmbientCol  () const {return m_ambient_color;      }
    const video::SColorf& getDiffuseCol  () const {return m_diffuse_color;      }
    const video::SColorf& getSpecularCol () const {return m_specular_color;     }
    const video::SColorf& getFogColor    () const {return m_fog_color;          }
    const video::SColorf& getSkyColor    () const {return m_sky_color;          }
    const bool&   useFog                 () const {return m_use_fog;            }
    const float&  getFogDensity          () const {return m_fog_density;        }
    const float&  getFogStart            () const {return m_fog_start;          }
    const float&  getFogEnd              () const {return m_fog_end;            }
    const std::string& getDescription    () const {return m_description;        }
    const std::string& getDesigner       () const {return m_designer;           }
    const std::string& getScreenshotFile () const {return m_screenshot;         }
    const std::string& getItemStyle      () const {return m_item_style;         }
    bool               hasFinalCamera    () const {return m_has_final_camera;   }
    const Vec3&        getCameraPosition () const {return m_camera_final_position;}
    const Vec3&        getCameraHPR      () const {return m_camera_final_hpr;   }
    btTransform        getStartTransform (unsigned int pos) const;
    void               getTerrainInfo(const Vec3 &pos, float *hot, Vec3* normal,
                                      const Material **material) const;
    float              getTerrainHeight(const Vec3 &pos) const;
    void               createPhysicsModel();
    void               update(float dt);
	void               reset();
    void               handleExplosion(const Vec3 &pos, const PhysicalObject *mp) const;

    /** Returns the graph of quads, mainly for the AI. */
    const QuadGraph&   getQuadGraph() const { return *m_quad_graph; }

    /** Returns 'a' angle for quad n. This angle is used to position a kart
     *  after a rescue, and to detect wrong directions. This function will
     *  always return the angle towards the first successor, i.e. the angle
     *  in the direction of the default way on the track.
     *  \param n Number of the quad for which the angle is asked. 
     */
    float              getAngle(int n) const 
                                   { return m_quad_graph->getAngleToNext(n, 0); }
    /*
    void               glVtx             (sgVec2 v, float x_offset, float y_offset) const
    {
        glVertex2f(
            x_offset+(v[0]-m_driveline_min[0])*m_scale_x,
            y_offset+(v[1]-m_driveline_min[1])*m_scale_y);
    }*/

private:
    void  loadTrack(const std::string &filename);
    void  itemCommand(const Vec3 &xyz, Item::ItemType item_type, 
                      int bNeedHeight);
    void  loadQuadGraph();
    void  readDrivelineFromFile(std::vector<Vec3>& line,
                                const std::string& file_ext);
    void  convertTrackToBullet(const scene::IMesh *mesh);
    void  getMusicInformation(std::vector<std::string>&             filenames, 
                              std::vector<MusicInformation*>& m_music   );
}
;   // class Track

#endif
