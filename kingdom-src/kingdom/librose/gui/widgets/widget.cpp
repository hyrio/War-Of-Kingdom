/* $Id: widget.cpp 54218 2012-05-19 08:46:15Z mordante $ */
/*
   Copyright (C) 2007 - 2012 by Mark de Wever <koraq@xs4all.nl>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#define GETTEXT_DOMAIN "wesnoth-lib"

#include "gui/widgets/settings.hpp"
#include "gui/widgets/window.hpp"
#include "gui/auxiliary/event/message.hpp"
#include "gui/dialogs/dialog.hpp"

namespace gui2 {

twidget::twidget()
	: id_("")
	, parent_(NULL)
	, x_(-1)
	, y_(-1)
	, w_(0)
	, h_(0)
	, dirty_(true)
	, volatile_(false)
	, visible_(VISIBLE)
	, drawing_action_(DRAWN)
	, clip_rect_()
	, fix_rect_(invalid_rect)
	, cookie_(NULL)
	, layout_size_(tpoint(0,0))
	, linked_group_()
	, debug_border_mode_(0)
	, debug_border_color_(0)
#ifdef DEBUG_WINDOW_LAYOUT_GRAPHS
	, last_best_size_(tpoint(0,0))
#endif
{
	DBG_GUI_LF << "widget create: " << static_cast<void*>(this) << "\n";
}

twidget::twidget(const tbuilder_widget& builder)
	: id_(builder.id)
	, parent_(NULL)
	, x_(-1)
	, y_(-1)
	, w_(0)
	, h_(0)
	, dirty_(true)
	, volatile_(false)
	, visible_(VISIBLE)
	, drawing_action_(DRAWN)
	, clip_rect_()
	, fix_rect_(invalid_rect)
	, cookie_(NULL)
	, layout_size_(tpoint(0,0))
	, linked_group_(builder.linked_group)
	, debug_border_mode_(builder.debug_border_mode)
	, debug_border_color_(builder.debug_border_color)
#ifdef DEBUG_WINDOW_LAYOUT_GRAPHS
	, last_best_size_(tpoint(0,0))
#endif
{
	DBG_GUI_LF << "widget create: " << static_cast<void*>(this) << "\n";
}

twidget::~twidget()
{
	twidget* p = parent();
	while (p) {
		fire(event::NOTIFY_REMOVAL, *p, NULL);
		p = p->parent();
	}

	twindow* window = get_window();

	if (window) {
		if (!linked_group_.empty()) {
			window->remove_linked_widget(linked_group_, this);
		}
		tdialog* dialog = window->dialog();
		if (dialog) {
			dialog->destruct_widget(this);
		}
	}
}

void twidget::set_id(const std::string& id)
{
	DBG_GUI_LF << "set id of " << static_cast<void*>(this)
		<< " to '" << id << "' "
		<< "(was '" << id_ << "'). Widget type: " <<
		(dynamic_cast<tcontrol*>(this) ?
			dynamic_cast<tcontrol*>(this)->get_control_type()
			: typeid(twidget).name())
		<< "\n";
	id_ = id;
}


void twidget::layout_init(const bool /*full_initialization*/)
{
	// assert(visible_ != INVISIBLE);
	assert(get_window());

	layout_size_ = tpoint(0,0);
	if (!linked_group_.empty()) {
		get_window()->add_linked_widget(linked_group_, this);
	}
}

tpoint twidget::get_best_size() const
{
	if (is_empty_rect(fix_rect_)) {
		tpoint result = layout_size_;
		if (result == tpoint(0, 0)) {
			result = calculate_best_size();
		}
		return result;

	} else {
		return tpoint(fix_rect_.w, fix_rect_.h);
	}
}

void twidget::place(const tpoint& origin, const tpoint& size)
{
	if (!is_valid_rect(fix_rect_)) {
		assert(size.x >= 0);
		assert(size.y >= 0);

		x_ = origin.x;
		y_ = origin.y;
		w_ = size.x;
		h_ = size.y;

	} else {
		x_ = fix_rect_.x;
		y_ = fix_rect_.y;
		w_ = fix_rect_.w;
		h_ = fix_rect_.h;
	}

	set_dirty();
}

void twidget::set_size(const tpoint& size)
{
	// release of visutal studio cannot detect assert, use breakpoint.
	assert(size.x >= 0);
	assert(size.y >= 0);

	w_ = size.x;
	h_ = size.y;

	set_dirty();
}

twidget* twidget::find_at(const tpoint& coordinate,
		const bool must_be_active)
{
	return is_at(coordinate, must_be_active) ? this : NULL;
}

const twidget* twidget::find_at(const tpoint& coordinate,
		const bool must_be_active) const
{
	return is_at(coordinate, must_be_active) ? this : NULL;
}

SDL_Rect twidget::get_dirty_rect() const
{
	return drawing_action_ == DRAWN
			? get_rect()
			: clip_rect_;
}

void twidget::move(const int x_offset, const int y_offset)
{
	x_ += x_offset;
	y_ += y_offset;
}

twindow* twidget::get_window()
{
	// Go up into the parent tree until we find the top level
	// parent, we can also be the toplevel so start with
	// ourselves instead of our parent.
	twidget* result = this;
	while (result->parent_) {
		result = result->parent_;
	}

	// on error dynamic_cast return 0 which is what we want.
	return dynamic_cast<twindow*>(result);
}

const twindow* twidget::get_window() const
{
	// Go up into the parent tree until we find the top level
	// parent, we can also be the toplevel so start with
	// ourselves instead of our parent.
	const twidget* result = this;
	while (result->parent_) {
		result = result->parent_;
	}

	// on error dynamic_cast return 0 which is what we want.
	return dynamic_cast<const twindow*>(result);
}

tdialog* twidget::dialog()
{
	twindow* window = get_window();
	return window ? window->dialog() : NULL;
}

void twidget::populate_dirty_list(twindow& caller,
		std::vector<twidget*>& call_stack)
{
	assert(call_stack.empty() || call_stack.back() != this);

	if (visible_ != VISIBLE) {
		return;
	}

	if (get_drawing_action() == NOT_DRAWN) {
		return;
	}

	call_stack.push_back(this);

	if (dirty_ || volatile_ || exist_anim()) {
		caller.add_to_dirty_list(call_stack);
	} else {
		// virtual function which only does something for container items.
		child_populate_dirty_list(caller, call_stack);
	}
}

void twidget::set_layout_size(const tpoint& size) 
{
	layout_size_ = size; 
}

void twidget::set_visible(const tvisible visible)
{
	if (visible == visible_) {
		return;
	}

	// Switching to or from invisible should invalidate the layout.
	const bool need_resize = visible_ == INVISIBLE || visible == INVISIBLE;
	visible_ = visible;

	if (need_resize) {
		twindow *window = get_window();
		if(window) {
			window->invalidate_layout();
		}
	}
	set_dirty();
}

twidget::tdrawing_action twidget::get_drawing_action() const
{
	return (w_ == 0 || h_ == 0)
		? NOT_DRAWN
		: drawing_action_;
}

void twidget::set_visible_area(const SDL_Rect& area)
{
	clip_rect_ = intersect_rects(area, get_rect());

	if(clip_rect_ == get_rect()) {
		drawing_action_ = DRAWN;
	} else if(clip_rect_ == empty_rect) {
		drawing_action_ = NOT_DRAWN;
	} else {
		drawing_action_ = PARTLY_DRAWN;
	}
}

void twidget::set_dirty(const bool dirty)
{
	dirty_ = dirty;
}

SDL_Rect twidget::calculate_blitting_rectangle(
		  const int x_offset
		, const int y_offset)
{
	SDL_Rect result = get_rect();
	result.x += x_offset;
	result.y += y_offset;
	return result;
}

SDL_Rect twidget::calculate_clipping_rectangle(SDL_Surface* surf, const int x_offset, const int y_offset)
{
	SDL_Rect clip_rect = clip_rect_;
	clip_rect.x += x_offset;
	clip_rect.y += y_offset;

	SDL_Rect surf_clip_rect;
	SDL_GetClipRect(surf, &surf_clip_rect);
	if (surf_clip_rect.w != surf->w || surf_clip_rect.h != surf->h) {
		return intersect_rects(clip_rect, surf_clip_rect);
	}
	return clip_rect;
}

void twidget::draw_background(surface& frame_buffer, int x_offset, int y_offset)
{
	assert(visible_ == VISIBLE);

	if(drawing_action_ == PARTLY_DRAWN) {
		const SDL_Rect clipping_rectangle = calculate_clipping_rectangle(frame_buffer, x_offset, y_offset);
		if (!clipping_rectangle.w || !clipping_rectangle.h) {
			return;
		}

		clip_rect_setter clip(frame_buffer, &clipping_rectangle);
		draw_debug_border(frame_buffer, x_offset, y_offset);
		impl_draw_background(frame_buffer, x_offset, y_offset);
	} else {
		draw_debug_border(frame_buffer, x_offset, y_offset);
		impl_draw_background(frame_buffer, x_offset, y_offset);
	}
}

void twidget::draw_children(surface& frame_buffer, int x_offset, int y_offset)
{
	assert(visible_ == VISIBLE);

	if (drawing_action_ == PARTLY_DRAWN) {
		const SDL_Rect clipping_rectangle = calculate_clipping_rectangle(frame_buffer, x_offset, y_offset);
		if (!clipping_rectangle.w || !clipping_rectangle.h) {
			return;
		}

		clip_rect_setter clip(frame_buffer, &clipping_rectangle);
		impl_draw_children(frame_buffer, x_offset, y_offset);
	} else {
		impl_draw_children(frame_buffer, x_offset, y_offset);
	}
}

void twidget::draw_foreground(surface& frame_buffer, int x_offset, int y_offset)
{
	assert(visible_ == VISIBLE);

	if (drawing_action_ == PARTLY_DRAWN) {
		const SDL_Rect clipping_rectangle = calculate_clipping_rectangle(frame_buffer, x_offset, y_offset);
		if (!clipping_rectangle.w || !clipping_rectangle.h) {
			return;
		}

		clip_rect_setter clip(frame_buffer, &clipping_rectangle);
		impl_draw_foreground(frame_buffer, x_offset, y_offset);
	} else {
		impl_draw_foreground(frame_buffer, x_offset, y_offset);
	}
}

void twidget::draw_debug_border(surface& frame_buffer)
{
	SDL_Rect r = drawing_action_ == PARTLY_DRAWN
		? clip_rect_
		: get_rect();
	switch(debug_border_mode_) {
		case 0:
			/* DO NOTHING */
			break;
		case 1:
			draw_rectangle(r.x, r.y, r.w, r.h
					, debug_border_color_, frame_buffer);
			break;
		case 2:
			sdl_fill_rect(frame_buffer, &r, debug_border_color_);
			break;
		default:
			assert(false);
	}
}

void twidget::draw_debug_border(
		  surface& frame_buffer
		, int x_offset
		, int y_offset)
{
	SDL_Rect r = drawing_action_ == PARTLY_DRAWN
		? calculate_clipping_rectangle(frame_buffer, x_offset, y_offset)
		: calculate_blitting_rectangle(x_offset, y_offset);

	switch(debug_border_mode_) {
		case 0:
			/* DO NOTHING */
			break;
		case 1:
			draw_rectangle(r.x, r.y, r.w, r.h
					, debug_border_color_, frame_buffer);
			break;
		case 2:
			sdl_fill_rect(frame_buffer, &r, debug_border_color_);
			break;
		default:
			assert(false);
	}
}

bool twidget::is_at(const tpoint& coordinate, const bool must_be_active) const
{
	if(visible_ == INVISIBLE
			|| (visible_ == HIDDEN && must_be_active)) {
		return false;
	}

	return coordinate.x >= x_
			&& coordinate.x < (x_ + static_cast<int>(w_))
			&& coordinate.y >= y_
			&& coordinate.y < (y_ + static_cast<int>(h_)) ? true : false;
}
} // namespace gui2
