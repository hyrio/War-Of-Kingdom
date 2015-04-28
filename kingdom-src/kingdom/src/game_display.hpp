/* $Id: game_display.hpp 47658 2010-11-21 13:59:16Z mordante $ */
/*
   Copyright (C) 2003 - 2010 by David White <dave@whitevine.net>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

/** @file */

#ifndef GAME_DISPLAY_H_INCLUDED
#define GAME_DISPLAY_H_INCLUDED

class config;
class tod_manager;
class play_controller;
class controller_base;

#include "animated.hpp"
#include "chat_events.hpp"
#include "hex_display.hpp"
#include "pathfind/pathfind.hpp"
#include "team.hpp"
#include "gui/widgets/report.hpp"

#include <deque>

// This needs to be separate from display.h because of the static
// singleton member, which will otherwise trigger link failure
// when building the editor.
typedef std::pair<int, std::set<map_location> > range_locs_pair;

namespace gui2 {
class treport;
}

class game_display : public hex_display
{
public:
	struct taccess_list {
		enum {TROOP = 0, HERO, TYPE_COUNT};

		taccess_list(unit_map& units, int type);
		void reset();
	
		void reload(game_display& gui, int side);
		void insert(game_display& gui, int side, void* cookie);
		void erase(game_display& gui, int side, void* cookie);
		void enable(game_display& gui, int side, void* cookie);
		void redraw(game_display& gui, int side, void* cookie);

		void calculate_require_count(int side);

		// for access_troops
		unit_map& units_;
		int type_;
		bool hide;
		gui2::ttabbar bar;

		bool reloaded;
	};

	game_display(unit_map& units, hero_map& heros, play_controller* controller, CVideo& video,
			const gamemap& map, const tod_manager& tod_manager,
			const std::vector<team>& t, const config& theme_cfg,
			const config& level);

	~game_display();

	std::string get_theme_patch() const;
	gui2::ttheme* create_theme_dlg(const config& cfg);
	void add_flag(int side_index, std::vector<std::string>& side_colors);

	/**
	 * Update lighting settings.
	 *
	 * Should be called on every new turn.
	 */
	void new_turn();

	/**
	 * Add r,g,b to the colors for all images displayed on the map.
	 *
	 * Used for special effects like flashes.
	 */
	void adjust_colors(int r, int g, int b);

	/**
	 * Function to display a location as selected.
	 *
	 * If a unit is in the location, and there is no unit in the currently
	 * highlighted hex, the unit will be displayed in the sidebar.
	 */
	virtual void select_hex(map_location hex);

	/**
	 * Function to highlight a location.
	 *
	 * If a unit is in the location, it will be displayed in the sidebar.
	 * Selection is used when a unit has been clicked on, while highlighting is
	 * used when a location has been moused over.
	 */
	virtual void highlight_hex(map_location hex);

	/**
	 * Change the unit to be displayed in the sidebar.
	 *
	 * This is used when selecting or highlighting is not wanted.
	 */
	void display_unit_hex(map_location hex);

	/**
	 * Sets the paths that are currently displayed as available
	 * for the unit to move along.
	 * All other paths will be grayed out.
	 */
	void highlight_reach(const pathfind::paths &paths_list);

	/**
	 * Add more paths to highlight.  Print numbers where they overlap.
	 * Used only by Show Enemy Moves.
	 */
	void highlight_another_reach(const pathfind::paths &paths_list);

	/** Reset highlighting of paths. */
	void unhighlight_reach();

	/**
	 * Sets the route along which footsteps are drawn to show movement of a
	 * unit. If NULL, no route is displayed. @a route does not have to remain
	 * valid after being set.
	 */
	void set_route(const pathfind::marked_route *route);

	/**
	 * Function to return 2 half-hex footsteps images for the given location.
	 * Only loc is on the current route set by set_route.
	 */
	std::vector<surface> footsteps_images(const map_location& loc);

	/** Draws the movement info (turns available) for a given location. */
	void draw_movement_info(const map_location& loc);

	void draw_report(int report_num);

	void pre_change_resolution(std::map<const std::string, bool>& actives);
	void post_change_resolution(const std::map<const std::string, bool>& actives);

	/** Rebuild all dynamic terrain. */
	void rebuild_all();

	/** Function to invalidate that unit status displayed on the sidebar. */
	void invalidate_unit() { invalidateUnit_ = true; }

	/** Same as invalidate_unit() if moving the displayed unit. */
	void invalidate_unit_after_move(const map_location& src, const map_location& dst);

	const time_of_day& get_time_of_day(const map_location& loc) const;

	bool has_time_area() const;
	bool unit_image_location_on(int x, int y);

	void goto_main_context_menu();
	void highlight_disctrict(const artifical& art);
	void unhighlight_disctrict();
	/** refresh field troop buttons */
	enum refresh_reason {REFRESH_RELOAD, REFRESH_ENABLE, REFRESH_INSERT, REFRESH_ERASE, REFRESH_DRAW, REFRESH_HIDE, REFRESH_CLEAR};

	void refresh_access_troops(int side, refresh_reason reason, void* cookie = NULL);
	void refresh_access_heros(int side, refresh_reason reason, void* cookie = NULL);
	bool access_is_null(int type) const;
	void resort_access_troops(unit& u);
	void verify_access_troops() const;

	void show_access_troop(gui2::ttabbar* bar, const gui2::tgrid::tchild& child);
	int current_list_type() const { return current_list_type_; }
	int next_list_type() const;
	void set_current_list_type(int type);
	taccess_list& type_2_list(int type);

	// runtime tooltip of mouse-over unit
	void show_unit_tip(const unit& troop, const map_location& loc);
	void show_tip(const std::string& message, const map_location& loc = map_location::null_location, bool clear = true);
	void hide_tip();

	/** Returns true if location (x,y) is covered in shroud. */
	bool shrouded(const map_location& loc) const {
		return viewpoint_ && viewpoint_->shrouded(loc);
	}

	/** Returns true if location (x,y) is covered in fog. */
	bool fogged(const map_location& loc) const {
		return viewpoint_ && viewpoint_->fogged(loc);
	}

	surface refresh_surface_report(int num, const reports::report& r, gui2::twidget& widget);
	void shrouded_and_fogged(const map_location& loc, bool& shrouded, bool& fogged) const;

protected:
	/**
	 * game_display pre_draw does specific things related e.g. to unit rendering
	 */
	void pre_draw(rect_of_hexes& hexes);

	/**
	 * Hex brightening for game - take units into account
	 */
	image::TYPE get_image_type(const map_location& loc);

	void redraw_units(const std::vector<map_location>& invalidated_unit_locations);

	void draw_invalidated();

	void post_commit();

	void draw_hex(const map_location& loc);

	/**
	 * Animated hex invalidation specific to gameplay
	 */
	void invalidate_animations();

	/**
	 * Extra game per-location invalidation (village ownership)
	 */
	void invalidate_animations_location(const map_location& loc);

	virtual surface minimap_surface(int w, int h);
	virtual void draw_minimap_units(surface& screen);

private:
	void refresh_access_list(int side, refresh_reason reason, void* cookie, int type);

public:
	/** Temporarily place a unit on map (moving: can overlap others).
	 *  The temp unit is added at the end of the temporary unit deque,
	 *  and therefore gets drawn last, over other units and temp units.
	 *  Adding the same unit twice isn't allowed.
	 */
	void place_temporary_unit(unit *u);

	/** Removes any instances of this temporary unit from the temporary unit vector.
	 *  Returns the number of temp units deleted (0 or 1, any other number indicates an error).
	 */
	void remove_temporary_unit();

	/**
	 * Allows a unit to request to be the only one drawn in its hex. Useful for situations where
	 * multiple units (one real, multiple temporary) can end up stacked, such as with the whiteboard.
	 * @param loc The location of the unit requesting exclusivity.
	 * @param unit The unit requesting exlusivity.
	 * @return false if there's already an exclusive draw request for this location.
	 */
	bool add_exclusive_draw(const map_location& loc, unit& unit);
	/**
	 * Cancels an exclusive draw request.
	 * @return The id of the unit whose exclusive draw request was canceled, or else
	 *         the empty string if there was no exclusive draw request for this location.
	 */
	std::string remove_exclusive_draw(const map_location& loc);

	/**
	 * Cancels all the exclusive draw requests.
	 */
	void clear_exclusive_draws() { exclusive_unit_draw_requests_.clear(); }

	/** Returns a reference to the temp units deque.
	 */
	const std::deque<unit*>& get_temp_units() { return temp_units_; }

	/** Place a city on map (expediting: can overlap others). */
	void place_expedite_city(artifical& city);
	void remove_expedite_city();

	/** Set the attack direction indicator. */
	void set_attack_indicator(unit* attack, bool browse = false);
	void clear_attack_indicator();
	std::set<map_location>& attack_indicator() { return attack_indicator_dst_; }
	const std::set<map_location>& attack_indicator() const { return attack_indicator_dst_; }
	bool loc_in_attack_indicator(const map_location& loc) const;
	std::vector<range_locs_pair>& attack_indicator_each() { return attack_indicator_each_dst_; }
	const std::vector<range_locs_pair>& attack_indicator_each() const { return attack_indicator_each_dst_; }
	const range_locs_pair& attack_indicator_each(std::string& str) const;

	void set_selectable_indicator(const std::set<map_location>& selectable) { selectable_indicator_ = selectable; }
	std::set<map_location>& selectable_indicator() { return selectable_indicator_; }
	const std::set<map_location>& selectable_indicator() const { return selectable_indicator_; }

	void set_hero_indicator(const hero& h);
	void clear_hero_indicator();
	void set_placable_indicator(unit& u);
	const std::set<map_location>& joinable_indicator() const { return joinable_indicator_; }
	const std::set<map_location>& placable_indicator() const { return placable_indicator_; }

	const std::pair<map_location, int>& tactic_indicator() const { return tactic_indicator_; }
	const std::pair<map_location, map_location>& formation_indicator() const { return formation_indicator_; }
	const map_location& interior_indicator() const { return interior_indicator_; }
	const std::set<map_location>& alternatable_indicator() const { return alternatable_indicator_; }
	const std::pair<map_location, int>& clear_formationed_indicator() const { return clear_formationed_indicator_; }
	const std::pair<map_location, int>& intervene_move_indicator() const { return intervene_move_indicator_; }

	const artifical* expedite_city() const { return expedite_city_; }

	/** Set the build direction indicator. */
	bool set_ea_build_indicator(const map_location& loc, bool set = true);
	void set_build_indicator(const unit* builder = NULL, const artifical* new_art = NULL);
	std::set<map_location>& build_indicator() { return build_indicator_dst_; }
	void clear_build_indicator();

	animation& insert_pass_scenario_anim(const animation& tpl);
	int pass_scenario_anim_id() const { return pass_scenario_anim_id_; }

	void erase_area_anim(int id);
	
	void set_terrain_dirty();

	/** Set the move source location. */
	void set_moving_src_loc(const map_location& loc);
	void clear_moving_src_loc();
	void set_moving_dst_loc(const map_location& loc);
	void clear_moving_dst_loc();

	/**
	 * Functions to add and remove overlays from locations.
	 *
	 * An overlay is an image that is displayed on top of the tile.
	 * One tile may have multiple overlays.
	 */
	void add_overlay(const map_location& loc, const std::string& image,
		const std::string& halo="", const std::string& team_name="",
		bool visible_under_fog = true);

	/** remove_overlay will remove all overlays on a tile. */
	void remove_overlay(const map_location& loc);

	/** remove_single_overlay will remove a single overlay from a tile */
	void remove_single_overlay(const map_location& loc, const std::string& toDelete);

	/**
	 * Check the overlay_map for proper team-specific overlays to be
	 * displayed/hidden
	 */
	void parse_team_overlays();

	// Functions used in the editor:

	//void draw_terrain_palette(int x, int y, terrain_type::TERRAIN selected);
	t_translation::t_terrain get_terrain_on(int palx, int paly, int x, int y);

	void send_notification(const std::string& owner, const std::string& message);

	/**
	 * Sets the team controlled by the player using the computer.
	 *
	 * Data from this team will be displayed in the game status.
	 * set_playing_team sets the team whose turn it currently is
	 */
	void set_team(size_t team, bool observe=false);
	void set_playing_team(size_t team);
	size_t get_playing_team() const {return activeTeam_;}
	const std::vector<team>& get_teams() {return teams_;}

	unit_map& get_units() {return units_;}
	const unit_map& get_const_units() const {return units_;}

	/**
	 * annotate hex with number, useful for debugging or UI prototype
	 */
	static int& debug_highlight(const map_location& loc);
	static void clear_debug_highlights() { debugHighlights_.clear(); }

	/** The viewing team is the team currently viewing the game. */
	size_t viewing_team() const { return currentTeam_; }
	int viewing_side() const { return currentTeam_ + 1; }

	/** The playing team is the team whose turn it is. */
	size_t playing_team() const { return activeTeam_; }

	bool team_valid() const { return currentTeam_ < teams_.size(); }
	std::string current_team_name() const;

	void add_observer(const std::string& name) { observers_.insert(name); }
	void remove_observer(const std::string& name) { observers_.erase(name); }
	const std::set<std::string>& observers() const { return observers_; }

	void add_chat_message(const time_t& time, const std::string& speaker,
		int side, const std::string& msg, events::chat_handler::MESSAGE_TYPE type, bool bell);
	void clear_chat_messages() { prune_chat_messages(true); }

	void begin_game();

	// virtual bool in_game() const { return in_game_; }
	bool in_theme() const { return true; }

	void draw_bar(const std::string& image, int xpos, int ypos,
		const map_location& loc, int size, double filled,
		const SDL_Color& col, fixed_t alpha, bool vtl = true);


	void construct_road_locs(const std::map<std::pair<int, int>, std::vector<map_location> >& roads);
	const std::map<map_location, std::vector<std::pair<artifical*, artifical*> > >& road_locs() const { return road_locs_; };
	enum {OWNER_NONE, OWNER_SELF, OWNER_ENEMY};
	int road_owner(std::map<map_location, std::vector<std::pair<artifical*, artifical*> > >::const_iterator& loc) const;
	virtual bool overlay_road_image(const map_location& loc, std::string& color_mod) const;

	/**
	 * Sets the linger mode for the display.
	 * There have been some discussions on what to do with fog and shroud
	 * the extra variables make it easier to modify the behaviour. There
	 * might even be a split between victory and defeat.
	 *
	 * @todo if the current implementation is wanted we can change
	 * the stuff back to a boolean
	 */
	enum tgame_mode {
		RUNNING,         /**< no linger overlay, show fog and shroud. */
		LINGER_SP,       /**< linger overlay, show fog and shroud. */
		LINGER_MP };     /**< linger overlay, show fog and shroud. */

	void set_game_mode(const tgame_mode game_mode);
	SDL_Rect clip_rect_commit() const;

private:
	game_display(const game_display&);
	void operator=(const game_display&);

	void draw_sidebar();
	void draw_game_status();

	// This surface must be freed by the caller
	surface get_flag(const map_location& loc);
	surface get_big_flag(const map_location& loc);

	unit_map& units_;
	hero_map& heros_;

	play_controller* controller_;

	/// collection of units destined to be drawn but not put into the unit map
	std::deque<unit*> temp_units_;

	typedef std::map<map_location, std::string> exclusive_unit_draw_requests_t;
	/// map of hexes where only one unit should be drawn, the one identified by the associated id string
	exclusive_unit_draw_requests_t exclusive_unit_draw_requests_;

	unit* temp_unit_;
	artifical* expedite_city_;

	int pass_scenario_anim_id_;

	bool terrain_dirty_;

	std::map<map_location, std::vector<std::pair<artifical*, artifical*> > > road_locs_;

	// Locations of the attack direction indicator's parts
	std::set<map_location> attack_indicator_dst_;
	std::vector<range_locs_pair> attack_indicator_each_dst_;

	std::set<map_location> selectable_indicator_;
	std::pair<map_location, int> tactic_indicator_;

	std::set<map_location> marker_indicator_dst_;
	std::pair<map_location, map_location> formation_indicator_;

	map_location interior_indicator_;
	std::set<map_location> alternatable_indicator_;
	std::pair<map_location, int> clear_formationed_indicator_;
	std::pair<map_location, int> intervene_move_indicator_;

	std::set<map_location> joinable_indicator_;
	std::set<map_location> placable_indicator_;

	// Locations of the build direction indicator's parts
	std::set<map_location> build_indicator_dst_;

	pathfind::marked_route route_;

	const tod_manager& tod_manager_;

	const std::vector<team>& teams_;

	const config& level_;

	void invalidate_route();

	bool invalidateUnit_;

	struct overlay {
		overlay(const std::string& img, const std::string& halo_img,
		        int handle, const std::string& overlay_team_name, const bool fogged) : image(img), halo(halo_img),
				team_name(overlay_team_name), halo_handle(handle) , visible_in_fog(fogged){}
		std::string image;
		std::string halo;
		std::string team_name;
		int halo_handle;
		bool visible_in_fog;
	};

	typedef std::multimap<map_location,overlay> overlay_map;

	overlay_map overlays_;

	size_t currentTeam_, activeTeam_;

	double sidebarScaling_;

	bool first_turn_, in_game_;

	std::set<std::string> observers_;

	struct chat_message
	{
		chat_message(int speaker, int h) : speaker_handle(speaker), handle(h), created_at(SDL_GetTicks())
		{}

		int speaker_handle;
		int handle;
		Uint32 created_at;
	};

	void prune_chat_messages(bool remove_all=false);

	std::vector<chat_message> chat_messages_;

	// Tiles lit for showing where unit(s) can reach
	typedef std::map<map_location,unsigned int> reach_map;
	reach_map reach_map_;
	reach_map reach_map_old_;
	bool reach_map_changed_;
	void process_reachmap_changes();

	tgame_mode game_mode_;

	// for city district
	std::set<map_location> disctrict_;
	std::set<map_location> disctrict_old_;
	bool disctrict_changed_;
	void process_disctrict_changes();

	// for access_troops
	taccess_list troop_list_;
	taccess_list hero_list_;
	int current_list_type_;
	int access_list_side_;

	gui2::tcontext_menu* ctrl_bar_;

	map_location moving_src_loc_;
	map_location moving_dst_loc_;

	// For debug mode
	static std::map<map_location, int> debugHighlights_;

	/** Animated flags for each team */
	std::vector<animated<image::locator> > flags_;

	/** Animated big flags for each team */
	std::vector<animated<image::locator> > big_flags_;
	std::vector<std::map<std::string, surface> > big_flags_cache_;

	const team* viewpoint_;
	/**
	 * the tiles invalidated at last redraw,
	 * to simplify the cleaning up of tiles left by units
	 */
	// static game_display * singleton_;
};

namespace reports {
reports::report generate_report(int type,
	const team &viewing_team,
		       int current_side, int active_side,
		       const map_location& loc, const map_location& mouseover,
	const std::set<std::string> &observers,
		       const config& level, bool show_everything = false);
}

#endif
