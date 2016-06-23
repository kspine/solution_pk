
	const auto account_uuid = get_account_uuid();
namespace {
	Coord get_init_cluster_hint(){
		PROFILE_ME;

		const auto count_castles_in_clusters = [](Coord coord_hint){
			std::size_t castle_count = 0;
			std::vector<boost::shared_ptr<MapObject>> map_objects;
			WorldMap::get_map_objects_by_rectangle(map_objects, WorldMap::get_cluster_scope(coord_hint));
			for(auto it = map_objects.begin(); it != map_objects.end(); ++it){
				const auto &map_object = *it;
				const auto map_object_type_id = map_object->get_map_object_type_id();
				if(map_object_type_id != MapObjectTypeIds::ID_CASTLE){
					continue;
				}
				++castle_count;
			}
			return castle_count;
		};

		const auto old_limit = GlobalStatus::cast<std::uint64_t>(GlobalStatus::SLOT_INIT_SERVER_LIMIT);
		if(old_limit != 0){
			const auto cluster_x = GlobalStatus::cast<std::int64_t>(GlobalStatus::SLOT_INIT_SERVER_X);
			const auto cluster_y = GlobalStatus::cast<std::int64_t>(GlobalStatus::SLOT_INIT_SERVER_Y);
			const auto coord_hint = Coord(cluster_x, cluster_y);
			LOG_EMPERY_CENTER_DEBUG("Testing cluster: coord_hint = ", coord_hint);
			const auto cluster = WorldMap::get_cluster(coord_hint);
			if(!cluster){
				LOG_EMPERY_CENTER_WARNING("Cluster is gone! coord_hint = ", coord_hint);
				goto _reselect;
			}
			const auto castle_count = count_castles_in_clusters(coord_hint);
			LOG_EMPERY_CENTER_DEBUG("Number of castles on cluster: coord_hint = ", coord_hint, ", castle_count = ", castle_count);
			if(castle_count >= old_limit){
				LOG_EMPERY_CENTER_DEBUG("Max number of castles exceeded: castle_count = ", castle_count, ", old_limit = ", old_limit);
				goto _reselect;
			}
			return coord_hint;
		}
	_reselect:
		LOG_EMPERY_CENTER_INFO("Reselecting init cluster server...");
		boost::container::flat_map<Coord, boost::shared_ptr<ClusterSession>> clusters;
		WorldMap::get_all_clusters(clusters);
		LOG_EMPERY_CENTER_DEBUG("Number of clusters is ", clusters.size());

		boost::container::flat_multimap<std::size_t, Coord> clusters_by_castle_count;
		clusters_by_castle_count.reserve(clusters.size());
		for(auto it = clusters.begin(); it != clusters.end(); ++it){
			const auto cluster_coord = it->first;
			const auto castle_count = count_castles_in_clusters(cluster_coord);
			LOG_EMPERY_CENTER_INFO("Number of castles on cluster: cluster_coord = ", cluster_coord, ", castle_count = ", castle_count);
			clusters_by_castle_count.emplace(castle_count, cluster_coord);
		}
		const auto limit_max = get_config<std::uint64_t>("cluster_map_castle_limit_max", 700);
		clusters_by_castle_count.erase(clusters_by_castle_count.lower_bound(limit_max), clusters_by_castle_count.end());

		if(clusters_by_castle_count.empty()){
			DEBUG_THROW(Exception, sslit("No clusters available"));
		}
		const auto front_it = clusters_by_castle_count.begin();

		const auto limit_init = get_config<std::uint64_t>("cluster_map_castle_limit_init", 300);
		auto new_limit = limit_init;
		if(front_it->first >= limit_init){
			const auto limit_increment = get_config<std::uint64_t>("cluster_map_castle_limit_increment", 200);
			new_limit = saturated_add(new_limit, saturated_mul((front_it->first - limit_init) / limit_increment + 1, limit_increment));
		}
		LOG_EMPERY_CENTER_DEBUG("Resetting castle limit: old_limit = ", old_limit, ", new_limit = ", new_limit);

		auto limit_str = boost::lexical_cast<std::string>(new_limit);
		auto x_str     = boost::lexical_cast<std::string>(front_it->second.x());
		auto y_str     = boost::lexical_cast<std::string>(front_it->second.y());
		GlobalStatus::set(GlobalStatus::SLOT_INIT_SERVER_LIMIT, std::move(limit_str));
		GlobalStatus::set(GlobalStatus::SLOT_INIT_SERVER_X,     std::move(x_str));
		GlobalStatus::set(GlobalStatus::SLOT_INIT_SERVER_Y,     std::move(y_str));

		LOG_EMPERY_CENTER_DEBUG("Selected cluster server: cluster_coord = ", front_it->second);
		return front_it->second;
	}
}


	const auto session = PlayerSessionMap::get(account_uuid);
	if(session){
		try {
			Msg::SC_AccountAttributes msg;
			msg.account_uuid    = account_uuid.str();
			msg.nick            = m_obj->unlocked_get_nick();
			msg.promotion_level = get_promotion_level();
			msg.activated       = has_been_activated();
			session->send(msg);
		} catch(std::exception &e){
			LOG_EMPERY_CENTER_WARNING("std::exception thrown: what = ", e.what());
			session->shutdown(e.what());
		}
	}


	
	if(has_been_activated()
	if(has_been_activated()){
		return;
	}

	const auto utc_now = Poseidon::get_utc_time();

	boost::shared_ptr<Castle> castle;
	std::vector<boost::shared_ptr<MapObject>> map_objects;
	WorldMap::get_map_objects_by_owner(map_objects, get_account_uuid());
	for(auto it = map_objects.begin(); it != map_objects.end(); ++it){
		const auto &map_object = *it;
		if(map_object->get_map_object_type_id() != MapObjectTypeIds::ID_CASTLE){
			continue;
		}
		const auto test_castle = boost::dynamic_pointer_cast<Castle>(map_object);
		if(test_castle){
			castle = std::move(test_castle);
			break;
		}
	}
	if(!castle){
		castle = WorldMap::create_init_castle(
			[&](Coord coord){
				return boost::make_shared<Castle>(MapObjectUuid(Poseidon::Uuid::random()),
					get_account_uuid(), MapObjectUuid(), get_nick(), coord, utc_now);
			},
			get_init_cluster_hint()
		);
		if(!castle){
			LOG_EMPERY_CENTER_WARNING("Failed to create initial castle! account_uuid = ", get_account_uuid());
			DEBUG_THROW(Exception, sslit("Failed to create initial castle"));
		}
	}

	m_obj->set_activated(true);
}
