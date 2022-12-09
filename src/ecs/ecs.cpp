#include "ecs/ecs.h"

namespace ecs {

void World::init() { ecs_id_count = 0; }

Entity World::entity() {
  // add empty entity into empty archetype
  EntityId new_entity_id = ecs_id_count++;
  auto& empty_archetype = archetype(Type());
  Record record(archetype(Type()), 0);
  entity_index.emplace(new_entity_id, std::move(record));
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

Archetype& World::add_to_archetype(Archetype& src_archetype, ComponentId component_id) {
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
  }
  return archetype_iter->second;
}

void World::add_component(EntityId entity_id, ComponentId component_id, std::any value) {
  Record& record = entity_index.at(entity_id);
  Archetype& src = record.archetype;
  Archetype& dst = add_to_archetype(src, component_id);

  assert(!component_index[component_id].contains(src.id) && "add multi-component is not supported");

  // empty entity (empty components) condition
  if (src.components.empty()) {
    for (int i = 0; i < dst.type.size(); i++) {
      size_t row = dst.components[i].size();
      // append default value of component
      dst.components[i].emplace_back(std::move(value));
      entity_index.erase(entity_id);
      entity_index.emplace(entity_id, std::move(Record(dst, row)));
    }
    return;
  }

  auto src_row = src.entity_to_row[entity_id];
  // move rest component_row from src into dst_archetype
  for (int src_col = 0; src_col < src.type.size(); src_col++) {
    auto col = component_index[src.type[src_col]][dst.id].column;
    std::vector<std::any>&& data_row = src.get_entity_row(entity_id);
    dst.set_entity_row(entity_id, );
  }

  // add default component value into dst_archetype[dst_column][dst_row] first
  auto dst_col = component_index[component_id][dst.id].column;
  auto dst_row = dst.components[dst_col].size();
  dst.components[dst_col].emplace_back(std::move(value));
  // update entity_index, this will erase the previous Record&
  entity_index.erase(entity_id);
  entity_index.emplace(entity_id, Record(dst, dst_row));
}

std::any& World::get_component(EntityId entity_id, ComponentId component_id) {
  auto& record = entity_index.at(entity_id);
  auto& archetype = record.archetype;

  assert(component_index.contains(component_id) && "required component not exists.");
  assert(component_index[component_id].contains(archetype.id) && "required component not exists in this entity.");

  auto comp_col = component_index[component_id][archetype.id].column;
  auto entity_row = archetype.entity_to_row[entity_id];
  return archetype.components[comp_col][entity_row];
}

// return pair, first: if component exists, second:
auto World::has_component(EntityId entity_id, ComponentId component_id)
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

void World::set_component(EntityId entity_id, ComponentId component_id, std::any value) {
  auto res = has_component(entity_id, component_id);
  if (res.first) {
    auto& old_value = get_component(entity_id, component_id);
    old_value = value;
  } else {
    add_component(entity_id, component_id, std::move(value));
  }
}

} // namespace ecs