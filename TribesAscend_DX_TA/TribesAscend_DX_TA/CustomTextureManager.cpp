#include "CustomTextureManager.h"
#include <unordered_map>
#include <utility>

#include <iostream>
using namespace std;

unsigned int cantorPairingFunction(unsigned int a, unsigned int b) {
  return (unsigned int)(0.5 * (a + b) * (a + b + 1) + b);
}

namespace DX {
CustomTexture::CustomTexture(unsigned int ui_index_count,
                             unsigned int ui_stage,
                             void* vp_texture) {
  this->ui_index_count = ui_index_count;
  this->ui_stage = ui_stage;
  this->vp_texture = vp_texture;
  this->ui_cantor_pairing_value =
      cantorPairingFunction(ui_index_count, ui_stage);
}

void* CustomTextureManager::tick(unsigned int ui_index_count,
                                 unsigned int ui_stage) {
  unordered_map<unsigned int, CustomTexture>::iterator iter_custom_textures =
      um_custom_textures.find(cantorPairingFunction(ui_index_count, ui_stage));

  if (iter_custom_textures != um_custom_textures.end()) {
    return (iter_custom_textures->second).vp_texture;
  } else {
    return NULL;
  }
}

void CustomTextureManager::addCustomTexture(unsigned int ui_index_count,
                                            unsigned int ui_stage,
                                            void* vp_texture) {
  CustomTexture o_custom_texture(ui_index_count, ui_stage, vp_texture);
  pair<unsigned int, CustomTexture> pair_custom_texture(
      cantorPairingFunction(ui_index_count, ui_stage), o_custom_texture);
  um_custom_textures.insert(pair_custom_texture);
}

CustomTextureManager o_custom_texture_manager;

}  // namespace DX

namespace DX {
CustomTextureStaged::CustomTextureStaged(unsigned int ui_prim_count) {
  this->ui_prim_count = ui_prim_count;
  for (int i = 0; i < TEXTURE_STAGES; i++) {
    this->setStageTexture(i, NULL);
    this->disableStage(i);
    this->v_texture_stages_texture_file_absolute_path.push_back("");
  }
}

void CustomTextureStaged::enableStage(unsigned int ui_stage) {
  this->b_texture_stages_custom[ui_stage] = true;
}

void CustomTextureStaged::disableStage(unsigned int ui_stage) {
  this->b_texture_stages_custom[ui_stage] = false;
}

bool CustomTextureStaged::usingStage(unsigned int ui_stage) {
  // cout << "UsingStage for " << this->ui_prim_count << ":" << ui_stage << " =
  // "
  //     << b_texture_stages_custom[ui_stage] << endl;
  return b_texture_stages_custom[ui_stage];
}

unsigned int** CustomTextureStaged::getStagesTextures(void) {
  return this->v_texture_stages;
}

void CustomTextureStaged::setStageTexture(unsigned int ui_stage,
                                          void* vp_texture) {
  this->v_texture_stages[ui_stage] = (unsigned int*)vp_texture;
  enableStage(ui_stage);
}

/*
unsigned int** CustomTextureStagedManager::tick(unsigned int ui_prim_count) {
  unordered_map<unsigned int, CustomTextureStaged>::iterator
      iter_custom_textures = um_custom_textures.find(ui_prim_count);

  if (iter_custom_textures != um_custom_textures.end()) {
    return (iter_custom_textures->second).getStagesTextures();
  } else {
    return NULL;
  }
}*/

CustomTextureStaged* CustomTextureStagedManager::tick(
    unsigned int ui_prim_count) {
  unordered_map<unsigned int, CustomTextureStaged>::iterator
      iter_custom_textures = um_custom_textures.find(ui_prim_count);

  if (iter_custom_textures != um_custom_textures.end()) {
    return &(iter_custom_textures->second);
  } else {
    return NULL;
  }
}

CustomTextureStaged* CustomTextureStagedManager::addCustomTexture(
    unsigned int ui_prim_count,
    unsigned int ui_stage,
    void* vp_texture) {
  unordered_map<unsigned int, CustomTextureStaged>::iterator
      iter_custom_textures = um_custom_textures.find(ui_prim_count);

  if (iter_custom_textures == um_custom_textures.end()) {
    // Custom texture does not exist in the map
    CustomTextureStaged o_custom_texture_staged(ui_prim_count);
    iter_custom_textures =
        um_custom_textures
            .insert(make_pair(ui_prim_count, o_custom_texture_staged))
            .first;
  }

  iter_custom_textures->second.setStageTexture(ui_stage, vp_texture);

  return &(iter_custom_textures->second);
}

CustomTextureStaged* CustomTextureStagedManager::addCustomTexture(
    unsigned int ui_prim_count,
    unsigned int ui_stage,
    void* vp_texture,
    string s_texture_aboslute_path) {
  unordered_map<unsigned int, CustomTextureStaged>::iterator
      iter_custom_textures = um_custom_textures.find(ui_prim_count);

  if (iter_custom_textures == um_custom_textures.end()) {
    // Custom texture does not exist in the map
    CustomTextureStaged o_custom_texture_staged(ui_prim_count);
    iter_custom_textures =
        um_custom_textures
            .insert(make_pair(ui_prim_count, o_custom_texture_staged))
            .first;
  }

  iter_custom_textures->second.setStageTexture(ui_stage, vp_texture);
  iter_custom_textures->second.setStageFilePath(ui_stage,
                                                s_texture_aboslute_path);

  return &(iter_custom_textures->second);
}

CustomTextureStaged* CustomTextureStagedManager::removeCustomTexture(
    unsigned int ui_prim_count,
    unsigned int ui_stage) {
  unordered_map<unsigned int, CustomTextureStaged>::iterator
      iter_custom_textures = um_custom_textures.find(ui_prim_count);

  if (iter_custom_textures != um_custom_textures.end()) {
    iter_custom_textures->second.setStageTexture(ui_stage, NULL);
  } else {
    return NULL;
  }

  bool b_texture_stages_empty = true;
  unsigned int** uipp_texture_stages =
      iter_custom_textures->second.getStagesTextures();

  for (int i = 0; i < TEXTURE_STAGES; i++) {
    if (uipp_texture_stages[i] != NULL) {
      b_texture_stages_empty = false;
    }
  }

  if (b_texture_stages_empty) {
    um_custom_textures.erase(iter_custom_textures);
  }

  return NULL;
}

vector<string>* CustomTextureStaged::getStagesFileNames(void) {
  return &v_texture_stages_texture_file_absolute_path;
}

CustomTextureStaged* CustomTextureStagedManager::getCustomTextureByName(
    string s_texture_name) {
  return NULL;
}

void CustomTextureStaged::setStageFilePath(unsigned int ui_stage,
                                           string s_texture_aboslute_path) {
  this->v_texture_stages_texture_file_absolute_path[ui_stage] =
      s_texture_aboslute_path;
}

void CustomTextureStagedManager::save(string s_file_name) {
  FILE* f_out = fopen(s_file_name.c_str(), "w");

  for (unordered_map<unsigned int, CustomTextureStaged>::iterator i =
           um_custom_textures.begin();
       i != um_custom_textures.end(); i++) {
    int i_vertex_count = i->first;

    unsigned int** v_stage_textures = i->second.getStagesTextures();
    vector<string>* v_stage_files_names = i->second.getStagesFileNames();

    for (int stage = 0; stage < TEXTURE_STAGES; stage++) {
      if (!i->second.usingStage(stage))
        continue;
      fprintf(f_out, "%d %d ", i_vertex_count, stage);
      if (v_stage_textures[stage] == NULL) {
        fprintf(f_out, "%s\n", "<NONE>");
      } else {
        fprintf(f_out, "%s\n", (*v_stage_files_names)[stage].c_str());
      }
    }
  }

  fclose(f_out);
}

void CustomTextureStagedManager::load(string s_file_name) {}

CustomTextureStagedManager o_custom_texture_staged_manager;

}  // namespace DX