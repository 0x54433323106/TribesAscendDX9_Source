#pragma once

#include <unordered_map>
#include <utility>

#include "Files.h"

using namespace std;

unsigned int cantorPairingFunction(unsigned int a, unsigned int b);

#define TEXTURE_STAGES 8

namespace DX {

class CustomTexture {
 public:
  unsigned int ui_index_count;
  unsigned int ui_stage;
  void* vp_texture;
  unsigned int ui_cantor_pairing_value;

  CustomTexture(unsigned int ui_index_count,
                unsigned int ui_stage,
                void* vp_texture);
};

class CustomTextureManager {
 public:
  unordered_map<unsigned int, CustomTexture> um_custom_textures;

  void* tick(unsigned int ui_index_count, unsigned ui_stage);

  void addCustomTexture(unsigned int ui_index_count,
                        unsigned int ui_stage,
                        void* vp_texture);
};
extern CustomTextureManager o_custom_texture_manager;

}  // namespace DX

namespace DX {

class CustomTextureStaged {
  unsigned int ui_prim_count;

  unsigned int* v_texture_stages[TEXTURE_STAGES];

  vector<string> v_texture_stages_texture_file_absolute_path;

  bool b_texture_stages_custom[TEXTURE_STAGES];

 public:
  CustomTextureStaged(unsigned int ui_prim_count);
  unsigned int** getStagesTextures(void);
  void setStageTexture(unsigned int ui_stage, void* vp_texture);
  void enableStage(unsigned int ui_stage);
  void disableStage(unsigned int ui_stage);
  bool usingStage(unsigned int ui_stage);
  void setStageFilePath(unsigned int ui_stage, string s_texture_aboslute_path);
  vector<string>* getStagesFileNames(void);
};

class CustomTextureStagedManager {
  unordered_map<unsigned int, CustomTextureStaged> um_custom_textures;

 public:
  // unsigned int** tick(unsigned int ui_index_count);

  CustomTextureStaged* tick(unsigned int ui_index_count);

  CustomTextureStaged* addCustomTexture(unsigned int ui_prim_count,
                                        unsigned int ui_stage,
                                        void* vp_texture);

  CustomTextureStaged* addCustomTexture(unsigned int ui_prim_count,
                                        unsigned int ui_stage,
                                        void* vp_texture, string s_texture_file_absolute_path);

  CustomTextureStaged* removeCustomTexture(unsigned int ui_prim_count,
                                           unsigned int ui_stage);

  CustomTextureStaged* getCustomTextureByName(string s_texture_name);

  void save(string s_file_name);

  void load(string s_file_name);
};

extern CustomTextureStagedManager o_custom_texture_staged_manager;
}  // namespace DX