/* $Id: controller_base.hpp 47608 2010-11-21 01:56:29Z shadowmaster $ */
/*
   Copyright (C) 2006 - 2010 by Joerg Hinrichs <joerg.hinrichs@alice-dsl.de>
   wesnoth playlevel Copyright (C) 2003 by David White <dave@whitevine.net>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#ifndef CONTROLLER_BASE_H_INCLUDED
#define CONTROLLER_BASE_H_INCLUDED

#include "global.hpp"

#include "key.hpp"
#include "gui/widgets/button.hpp"
class CVideo;

namespace gui2 {
class treport;
}

namespace events {
class mouse_handler_base;
}

class controller_base : public events::handler
{
public:
	enum DIRECTION {UP, DOWN, LEFT, RIGHT, NORTH_EAST, SOUTH_EAST, SOUTH_WEST, NORTH_WEST};

	controller_base(int ticks, const config& game_config, CVideo& video);
	virtual ~controller_base();

	void play_slice(bool is_delay_enabled = true);

	int get_ticks();

	bool in_browse() const { return browse_; }
	/**
	 * Get a reference to a mouse handler member a derived class uses
	 */
	virtual events::mouse_handler_base& get_mouse_handler_base() = 0;

	virtual bool in_context_menu(const std::string& id) const { return true; }
	virtual bool actived_context_menu(const std::string& id) const;
	virtual void prepare_show_menu(gui2::tbutton& widget, const std::string& id, int width, int height) const {}

	void execute_command(int command, const std::string& minor);

	virtual void execute_command2(int command, const std::string& sparam);
	virtual bool can_execute_command(int command, const std::string& sparam) const { return true; }

	// true: halt. false: continue to exectue base action.
	virtual bool toggle_tabbar(gui2::twidget* widget) { return true; }
	virtual void click_tabbar(gui2::twidget* widget, const std::string& sparam) {}

	/**
	 * Get a reference to a display member a derived class uses
	 */
	virtual display& get_display() = 0;
	virtual const display& get_display() const = 0;

protected:
	/**
	 * Called by play_slice after events:: calls, but before processing scroll
	 * and other things like menu.
	 */
	virtual void slice_before_scroll();

	/**
	 * Called at the very end of play_slice
	 */
	virtual void slice_end();

	/**
	 * Derived classes should override this to return false when arrow keys
	 * should not scroll the map, hotkeys not processed etc, for example
	 * when a textbox is active
	 * @returns true when arrow keys should scroll the map, false otherwise
	 */
	virtual bool have_keyboard_focus();

	/**
	 * Handle scrolling by keyboard and moving mouse near map edges
	 * @see is_keyboard_scroll_active
	 * @return true when there was any scrolling, false otherwise
	 */
	bool handle_scroll(CKey& key, int mousex, int mousey, int mouse_flags);

	/**
	 * Process mouse- and keypress-events from SDL.
	 * Not virtual but calls various virtual function to allow specialized
	 * behaviour of derived classes.
	 */
	void handle_event(const SDL_Event& event);

	/**
	 * Process keydown (only when the general map display does not have focus).
	 */
	virtual void process_focus_keydown_event(const SDL_Event& event);

	/**
	 * Process keydown (always).
	 * Overridden in derived classes
	 */
	virtual void process_keydown_event(const SDL_Event& event);

	/**
	 * Process keyup (always).
	 * Overridden in derived classes
	 */
	virtual void process_keyup_event(const SDL_Event& event);

	/**
	 * Called after processing a mouse button up or down event
	 * Overridden in derived classes
	 */
	virtual void post_mouse_press(const SDL_Event& event);

	bool handle_scroll_wheel(int dx, int dy, int hit_threshold, int motion_threshold);
	const config &get_theme(const config& game_config, std::string theme_name);

	const config& game_config_;
	const int ticks_;
	CKey key_;
	bool browse_;
	bool scrolling_;

	// finger motion result to scroll
	bool finger_motion_scroll_;
	int finger_motion_direction_;
	bool wait_bh_event_;
};


#endif
