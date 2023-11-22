#include "spider.h"
#include <Mango/mango.h>
#include <Mango/render/drawing.h>
#include <Mango/render/framedata.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Debugging
// -----------------------------------------------------------------------------

const bool DEBUG_ENABLE_FPS_COUNT = true;
const bool DEBUG_USE_WIREFRAME = false;
const bool DEBUG_USE_RASTERIZE = true;
const bool DEBUG_VIEW_NORMALS = false;
const bool DEBUG_SDF_ENABLE = true;

// -----------------------------------------------------------------------------
// Lights
const Vec3 COLLOR_PALLETE[7] = {
    {{1.0f, 0.0f, 0.0f}}, // Red
    {{0.0f, 1.0f, 0.0f}}, // Green
    {{0.0f, 0.0f, 1.0f}}, // Blue
    {{1.0f, 1.0f, 0.0f}}, // Yellow
    {{1.0f, 1.0f, 1.0f}}, // White
    {{0.3f, 0.0f, 0.5f}}, // Indigo
    {{0.5f, 0.0f, 0.5f}}  // Violet
};

int POINT_LIGHTS_BEGIN = 3;
int POINT_LIGHTS_END = 6;

// Scene data
Scene scene;
Vec4 slight_right;

Camera init_camera(int frame_width, int frame_height) {
  Camera cam;
  cam.game_object = game_object_default();
  cam.game_object.position = (Vec3){{0.0f, 1.0f, 20.0f}};
  cam.dirty_local = true;
  cam.fov = 50.0f;
  cam.aspect = (float)(frame_width) / frame_height;
  cam.z_near = 0.1f;
  cam.z_far = 1000.0f;
  cam.width = frame_width;
  cam.height = frame_height;
  return cam;
}

int alloc_objects(Scene *scene) {
  // Vec3 white = COLLOR_PALLETE[4];
  // Vec3 blue = (Vec3){{0.0f, 0.5f, 1.0f}};

  // Objects
  int manual_objects = 7;
  scene->object_count = manual_objects + spider_object_amt;
  scene->dirty_locals = (bool *)malloc(scene->object_count * sizeof(bool));
  if (scene->dirty_locals == NULL) {
    printf("alloc_objects dirty locals malloc failed");
    return 1;
  }

  for (int i = 0; i < scene->object_count; ++i) {
    scene->dirty_locals[i] = true;
  }

  scene->attributes =
      (GameObjectAttr *)malloc(scene->object_count * sizeof(GameObjectAttr));
  if (scene->attributes == NULL) {
    printf("alloc_objects attributes malloc failed");
    return 1;
  }

  scene->objects =
      (GameObject *)malloc(scene->object_count * sizeof(GameObject));
  if (scene->objects == NULL) {
    printf("alloc_objects game objects malloc failed");
    return 1;
  }

  float light_intensity = 50.0f;
  for (int i = POINT_LIGHTS_BEGIN; i < POINT_LIGHTS_END; ++i) {
    scene->objects[i] = game_object_default();
    scene->attributes[i].type = ATTR_LIGHT;
    scene->attributes[i].light.type = LIGHT_POINT;
    scene->attributes[i].light.color = COLLOR_PALLETE[i];
    scene->attributes[i].light.intensity = light_intensity;
  }

  // scene->objects[6] = game_object_default();
  // scene->attributes[6].type = ATTR_LIGHT;
  // scene->attributes[6].light.type = LIGHT_AMBIENT;
  // scene->attributes[6].light.color = (Vec3){{0.4f, 0.4f, 0.4f}};
  // scene->attributes[6].light.intensity = light_intensity;

  memcpy(scene->objects + manual_objects, spider_game_objects,
         spider_object_amt * sizeof(GameObject));
  memcpy(scene->attributes + manual_objects, spider_attrs,
         spider_object_amt * sizeof(GameObjectAttr));
  scene->max_depth = MAX(scene->max_depth, spider_max_depth);

  // scene->objects[manual_objects].scale = (Vec3){{0.1f, 0.1f, 0.1f}};

  return 0;
}

Mango *mango;
Vec4 slight_right;
Real attack_cd = 0;

void update(Real dt) {
  static float frames = 0;
  attack_cd += dt;
  if (attack_cd > 3000) {
    mango_play_anim(mango, 7, &spider_Attack_anim);
    attack_cd = 0;
  }

  // Update object(s)
  // quat_mul(&scene.camera.game_object.quaternion, &slight_right);
  // scene.camera.game_object.needs_update = true;
  // End

  quat_mul(&scene.objects[7].quaternion, &slight_right);
  scene.dirty_locals[7] = true;
  float circle_radius = 10.0f;
  int num_lights = POINT_LIGHTS_END - POINT_LIGHTS_BEGIN;
  float angle_increment = 2.0f * M_PI / num_lights;
  for (int i = POINT_LIGHTS_BEGIN; i < POINT_LIGHTS_END; ++i) {
    float angle = angle_increment * (i + (frames / 20.0f));
    float x = circle_radius * cosf(angle);
    float z = circle_radius * sinf(angle);
    scene.dirty_locals[i] = true;
    scene.objects[i].position = (Vec3){{x, 0.0f, z}};
  }

  // Rotate model
  quat_mul(&scene.objects[0].quaternion, &slight_right);
  scene.dirty_locals[0] = true;
  ++frames;
}

int main(int argc, char *argv[]) {
  // Start

  // Scene data
  slight_right = quat_from_axis(UNIT_Y, 0.01f);
  if (alloc_objects(&scene) != 0) {
    return 1;
  }

  Camera camera = init_camera(BG_W, BG_H);
  scene.camera = &camera;

  // Debug options
  scene.debug.use_wireframe = DEBUG_USE_WIREFRAME;
  scene.debug.use_rasterize = DEBUG_USE_RASTERIZE;
  scene.debug.view_normals = DEBUG_VIEW_NORMALS;
  scene.debug.sdf_enable = DEBUG_SDF_ENABLE;

  // Update loop

  mango = mango_alloc(&scene, "", 0, 0);
  if (mango == NULL) {
    return 1;
  }
  mango->user_update = &update;
  mango_run(mango);

  return 0;
}
