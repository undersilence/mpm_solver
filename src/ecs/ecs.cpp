#include "ecs/ecs.hpp"
#include <numeric>

namespace ecs {

void World::init() { ecs_id_count = 0; }

Entity World::entity() {
  // add empty entity into empty archetype
  ecs_id_t new_entity_id = ecs_id_count++;
  auto& empty_archetype = archetype(Type());
  empty_archetype.add_entity_row(new_entity_id, {});
  entity_index.emplace(new_entity_id, std::move(Record(empty_archetype)));
  return {*this, new_entity_id};
}

Archetype& World::archetype(const Type& type) {
  // check if archetype_index exists
  auto archetype_iter = archetype_index.find(type);
  if (archetype_iter == archetype_index.end()) {
    // if not exists, create new archetype
    Archetype archetype(*this, ecs_id_count++, type);
    for (size_t i = 0; i < type.size(); ++i) {
      component_index[type[i]][archetype.id] = ArchetypeRecord{i};
    }
    auto p = archetype_index.emplace(std::make_pair(type, std::move(archetype)));
    assert(p.second && "emplace new archetype failed.");
    return p.first->second;
  } else {
    return archetype_iter->second;
  }
}

Archetype& World::add_to_archetype(Archetype& src_archetype, ecs_id_t component_id) {
  // assume that src_archetype not contains this component by default
  auto dst_type = src_archetype.type;
  dst_type.insert(std::upper_bound(std::begin(dst_type), std::end(dst_type), component_id),
                  component_id);
  // check if dst_archetype is already created
  auto archetype_iter = archetype_index.find(dst_type);
  if (archetype_iter == archetype_index.end()) {
    // create new archetype and edges between src & dst
    Archetype& dst_archetype = archetype(dst_type);
    src_archetype.add_archetypes.emplace(component_id, dst_archetype);
    dst_archetype.del_archetypes.emplace(component_id, src_archetype);
    return dst_archetype;
  } else {
    Archetype& dst_archetype = archetype_iter->second;
    src_archetype.add_archetypes.emplace(component_id, dst_archetype);
    dst_archetype.del_archetypes.emplace(component_id, src_archetype);
    return dst_archetype;
  }
}

Archetype& World::del_to_archetype(Archetype& src_archetype, ecs_id_t component_id) {
  return src_archetype;
}

void World::add_component(ecs_id_t entity_id, ecs_id_t component_id, std::any value) {
  Record& record = entity_index.at(entity_id);
  Archetype& src = record.archetype;
  Archetype& dst = add_to_archetype(src, component_id);

  assert(!component_index[component_id].contains(src.id) && "add multi-component is not supported");

  auto src_row = src.entity_to_row[entity_id];
  // move rest component_row from src into dst_archetype
  for (int src_col = 0; src_col < src.type.size(); src_col++) {
    auto dst_col = component_index[src.type[src_col]][dst.id].column;
    std::vector<std::any>&& data_row = src.get_entity_row(entity_id);
    std::any& src_value = src.components[src_col][src_row];
    dst.components[dst_col].emplace_back(std::move(src_value));
  }
  src.del_entity_row(entity_id);

  // add default component value into dst_archetype[dst_column][dst_row] first
  auto dst_col = component_index[component_id][dst.id].column;
  auto dst_row = dst.components[dst_col].size();
  dst.components[dst_col].emplace_back(std::move(value));
  // update entity_index, this will erase the previous Record&

  dst.entity_to_row[entity_id] = dst_row;
  dst.row_to_entity[dst_row] = entity_id;
  entity_index.erase(entity_id);
  entity_index.emplace(entity_id, Record(dst));

  assert(src.entity_to_row.size() == src.row_to_entity.size() &&
         "please check the entity <=> row pair in src archetype.");
  assert(dst.entity_to_row.size() == dst.row_to_entity.size() &&
         "please check the entity <=> row pair in dst archetype.");
}

std::any& World::get_component(ecs_id_t entity_id, ecs_id_t component_id) {
  auto& record = entity_index.at(entity_id);
  auto& archetype = record.archetype;

  assert(component_index.contains(component_id) && "required component not exists.");
  assert(component_index[component_id].contains(archetype.id) &&
         "required component not exists in this entity.");

  auto col = component_index[component_id][archetype.id].column;
  auto row = archetype.entity_to_row[entity_id];
  return archetype.components[col][row];
}

// return pair, first: if component exists, second:
auto World::has_component(ecs_id_t entity_id, ecs_id_t component_id)
    -> std::pair<bool, World::ArchetypeMap::iterator> {
  auto& record = entity_index.at(entity_id);
  auto& archetype = record.archetype;
  auto map_iter = component_index.find(component_id);
  if (map_iter == component_index.end()) {
    return {false, {}};
  }
  auto& archetype_map = component_index[component_id];
  auto iter = archetype_map.find(archetype.id);
  return std::make_pair(iter != archetype_map.end(), iter);
}

void World::set_component(ecs_id_t entity_id, ecs_id_t component_id, std::any value) {
  auto res = has_component(entity_id, component_id);
  if (res.first) {
    auto& old_value = get_component(entity_id, component_id);
    old_value = value;
  } else {
    add_component(entity_id, component_id, std::move(value));
  }
}

void World::del_component(ecs_id_t entity_id, ecs_id_t component_id) {
  auto& record = entity_index.at(entity_id);
  auto& archetype = record.archetype;
}

// add multiple components at once
void World::add_components(ecs_id_t entity_id,
                           std::vector<ecs_id_t> component_ids,
                           std::vector<std::any> values) {
  assert(component_ids.size() == values.size() &&
         "the size of values and components size are not the same.");
  auto& src = entity_index.at(entity_id).archetype;
  // initialized dst archetype type info
  auto dst_type = src.type;
  dst_type.insert(dst_type.begin(), component_ids.begin(), component_ids.end());
  std::sort(dst_type.begin(), dst_type.end());

  auto archetype_iter = archetype_index.find(dst_type);
  if (archetype_iter != archetype_index.end()) {
    auto& dst = archetype_iter->second;

    auto src_row = src.entity_to_row[entity_id];
    // move rest component_row from src into dst_archetype
    for (int src_col = 0; src_col < src.type.size(); src_col++) {
      auto dst_col = component_index[src.type[src_col]][dst.id].column;
      std::vector<std::any>&& data_row = src.get_entity_row(entity_id);
      std::any& src_value = src.components[src_col][src_row];
      dst.components[dst_col].emplace_back(std::move(src_value));
    }
    src.del_entity_row(entity_id);

    for (size_t i = 0; i < component_ids.size(); ++i) {
      // add default component value into dst_archetype[dst_column][dst_row] first
      auto dst_col = component_index[component_ids[i]][dst.id].column;
      dst.components[dst_col].emplace_back(std::move(values[i]));
    }

    // update entity_index, this will erase the previous Record&
    auto dst_row = dst.row();
    dst.entity_to_row[entity_id] = dst_row;
    dst.row_to_entity[dst_row] = entity_id;

    entity_index.erase(entity_id);
    entity_index.emplace(entity_id, Record(dst));
    printf("add components directly!\n");
  } else {
    for(size_t i = 0; i < component_ids.size(); ++i) {
      // add component and build archetype graph one by one
      add_component(entity_id, component_ids[i], values[i]);
    }
  }
}

void World::set_components(ecs_id_t entity_id, std::vector<ecs_id_t> component_ids,
                           std::vector<std::any> values) {
  std::vector<ecs_id_t> new_component_ids;
  std::vector<std::any> new_values;
  std::vector<size_t> set_indices;

  // dispatch these operations by adding and setting
  for(int i = 0 ; i < component_ids.size(); ++i) {
    if (has_component(entity_id, component_ids[i]).first) {
      new_component_ids.emplace_back(component_ids[i]);
      new_values.emplace_back(std::move(values[i]));
    } else {
      set_indices.emplace_back(i);
    }
  }

  if (!new_component_ids.empty()) {
    add_components(entity_id, std::move(new_component_ids), std::move(new_values));
  }
  for (auto i : set_indices) {
    set_component(entity_id, component_ids[i], values[i]);
  }
}


} // namespace ecs