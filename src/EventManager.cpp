/*
Copyright © 2013 Igor Paliychuk

This file is part of FLARE.

FLARE is free software: you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

FLARE is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
FLARE.  If not, see http://www.gnu.org/licenses/
*/

#include "EventManager.h"
#include "UtilsParsing.h"

EventManager::EventManager() {

}

EventManager::~EventManager() {

}

void EventManager::loadEvent(FileParser &infile, Map_Event* evnt) {
	if (!evnt) return;

	if (infile.key == "type") {
		// @ATTR event.type|[on_trigger:on_mapexit:on_leave:on_load:on_clear]|Type of map event.
		std::string type = infile.val;
		evnt->type = type;

		if      (type == "on_trigger");
		else if (type == "on_mapexit"); // no need to set keep_after_trigger to false correctly, it's ignored anyway
		else if (type == "on_leave");
		else if (type == "on_load") {
			evnt->keep_after_trigger = false;
		}
		else if (type == "on_clear") {
			evnt->keep_after_trigger = false;
		}
		else {
			fprintf(stderr, "EventManager: Loading event in file %s\nEvent type %s unknown, change to \"on_trigger\" to suppress this warning.\n", infile.getFileName().c_str(), type.c_str());
		}
	}
	else if (infile.key == "location") {
		// @ATTR event.location|[x,y,w,h]|Defines the location area for the event.
		evnt->location.x = toInt(infile.nextValue());
		evnt->location.y = toInt(infile.nextValue());
		evnt->location.w = toInt(infile.nextValue());
		evnt->location.h = toInt(infile.nextValue());

		evnt->center.x = evnt->location.x + (float)evnt->location.w/2;
		evnt->center.y = evnt->location.y + (float)evnt->location.h/2;
	}
	else if (infile.key == "hotspot") {
		//  @ATTR event.hotspot|[ [x, y, w, h] : location ]|Event uses location as hotspot or defined by rect.
		if (infile.val == "location") {
			evnt->hotspot.x = evnt->location.x;
			evnt->hotspot.y = evnt->location.y;
			evnt->hotspot.w = evnt->location.w;
			evnt->hotspot.h = evnt->location.h;
		}
		else {
			evnt->hotspot.x = toInt(infile.nextValue());
			evnt->hotspot.y = toInt(infile.nextValue());
			evnt->hotspot.w = toInt(infile.nextValue());
			evnt->hotspot.h = toInt(infile.nextValue());
		}
	}
	else if (infile.key == "cooldown") {
		// @ATTR event.cooldown|duration|Duration for event cooldown.
		evnt->cooldown = parse_duration(infile.val);
	}
	else if (infile.key == "reachable_from") {
		// @ATTR event.reachable_from|[x,y,w,h]|If the hero is inside this rectangle, they can activate the event.
		evnt->reachable_from.x = toInt(infile.nextValue());
		evnt->reachable_from.y = toInt(infile.nextValue());
		evnt->reachable_from.w = toInt(infile.nextValue());
		evnt->reachable_from.h = toInt(infile.nextValue());
	}
	else {
		loadEventComponent(infile, evnt);
	}
}

void EventManager::loadEventComponent(FileParser &infile, Map_Event* evnt) {
	if (!evnt) return;

	// new event component
	evnt->components.push_back(Event_Component());
	Event_Component *e = &evnt->components.back();
	e->type = infile.key;

	if (infile.key == "tooltip") {
		// @ATTR event.tooltip|string|Tooltip for event
		e->s = msg->get(infile.val);
	}
	else if (infile.key == "power_path") {
		// @ATTR event.power_path|[hero:[x,y]]|Event power path
		// x,y are src, if s=="hero" we target the hero,
		// else we'll use values in a,b as coordinates
		e->x = toInt(infile.nextValue());
		e->y = toInt(infile.nextValue());

		std::string dest = infile.nextValue();
		if (dest == "hero") {
			e->s = "hero";
		}
		else {
			e->a = toInt(dest);
			e->b = toInt(infile.nextValue());
		}
	}
	else if (infile.key == "power_damage") {
		// @ATTR event.power_damage|min(integer), max(integer)|Range of power damage
		e->a = toInt(infile.nextValue());
		e->b = toInt(infile.nextValue());
	}
	else if (infile.key == "intermap") {
		// @ATTR event.intermap|[map(string),x(integer),y(integer)]|Jump to specific map at location specified.
		e->s = infile.nextValue();
		e->x = toInt(infile.nextValue());
		e->y = toInt(infile.nextValue());
	}
	else if (infile.key == "intramap") {
		// @ATTR event.intramap|[x(integer),y(integer)]|Jump to specific position within current map.
		e->x = toInt(infile.nextValue());
		e->y = toInt(infile.nextValue());
	}
	else if (infile.key == "mapmod") {
		// @ATTR event.mapmod|[string,int,int,int],..|Modify map tiles
		e->s = infile.nextValue();
		e->x = toInt(infile.nextValue());
		e->y = toInt(infile.nextValue());
		e->z = toInt(infile.nextValue());

		// add repeating mapmods
		std::string repeat_val = infile.nextValue();
		while (repeat_val != "") {
			evnt->components.push_back(Event_Component());
			e = &evnt->components.back();
			e->type = infile.key;
			e->s = repeat_val;
			e->x = toInt(infile.nextValue());
			e->y = toInt(infile.nextValue());
			e->z = toInt(infile.nextValue());

			repeat_val = infile.nextValue();
		}
	}
	else if (infile.key == "soundfx") {
		// @ATTR event.soundfx|[soundfile(string),x(integer),y(integer)]|Play a sound at optional location
		e->s = infile.nextValue();
		e->x = e->y = -1;

		std::string s = infile.nextValue();
		if (s != "") e->x = toInt(s);

		s = infile.nextValue();
		if (s != "") e->y = toInt(s);

	}
	else if (infile.key == "loot") {
		// @ATTR event.loot|[string,x(integer),y(integer),drop_chance([fixed:chance(integer)]),quantity_min(integer),quantity_max(integer)],...|Add loot to the event
		e->s = infile.nextValue();
		e->x = toInt(infile.nextValue());
		e->y = toInt(infile.nextValue());

		// drop chance
		std::string chance = infile.nextValue();
		if (chance == "fixed") e->z = 0;
		else e->z = toInt(chance);

		// quantity min/max
		e->a = toInt(infile.nextValue());
		if (e->a < 1) e->a = 1;
		e->b = toInt(infile.nextValue());
		if (e->b < e->a) e->b = e->a;

		// add repeating loot
		std::string repeat_val = infile.nextValue();
		while (repeat_val != "") {
			evnt->components.push_back(Event_Component());
			e = &evnt->components.back();
			e->type = infile.key;
			e->s = repeat_val;
			e->x = toInt(infile.nextValue());
			e->y = toInt(infile.nextValue());

			chance = infile.nextValue();
			if (chance == "fixed") e->z = 0;
			else e->z = toInt(chance);

			e->a = toInt(infile.nextValue());
			if (e->a < 1) e->a = 1;
			e->b = toInt(infile.nextValue());
			if (e->b < e->a) e->b = e->a;

			repeat_val = infile.nextValue();
		}
	}
	else if (infile.key == "msg") {
		// @ATTR event.msg|string|Adds a message to be displayed for the event.
		e->s = msg->get(infile.val);
	}
	else if (infile.key == "shakycam") {
		// @ATTR event.shakycam|integer|
		e->x = toInt(infile.val);
	}
	else if (infile.key == "requires_status") {
		// @ATTR event.requires_status|string,...|Event requires list of statuses
		e->s = infile.nextValue();

		// add repeating requires_status
		std::string repeat_val = infile.nextValue();
		while (repeat_val != "") {
			evnt->components.push_back(Event_Component());
			e = &evnt->components.back();
			e->type = infile.key;
			e->s = repeat_val;

			repeat_val = infile.nextValue();
		}
	}
	else if (infile.key == "requires_not_status") {
		// @ATTR event.requires_not|string,...|Event requires not list of statuses
		e->s = infile.nextValue();

		// add repeating requires_not
		std::string repeat_val = infile.nextValue();
		while (repeat_val != "") {
			evnt->components.push_back(Event_Component());
			e = &evnt->components.back();
			e->type = infile.key;
			e->s = repeat_val;

			repeat_val = infile.nextValue();
		}
	}
	else if (infile.key == "requires_level") {
		// @ATTR event.requires_level|integer|Event requires hero level
		e->x = toInt(infile.nextValue());
	}
	else if (infile.key == "requires_not_level") {
		// @ATTR event.requires_not_level|integer|Event requires not hero level
		e->x = toInt(infile.nextValue());
	}
	else if (infile.key == "requires_item") {
		// @ATTR event.requires_item|integer,...|Event requires specific item
		e->x = toInt(infile.nextValue());

		// add repeating requires_item
		std::string repeat_val = infile.nextValue();
		while (repeat_val != "") {
			evnt->components.push_back(Event_Component());
			e = &evnt->components.back();
			e->type = infile.key;
			e->x = toInt(repeat_val);

			repeat_val = infile.nextValue();
		}
	}
	else if (infile.key == "set_status") {
		// @ATTR event.set_status|string,...|Sets specified statuses
		e->s = infile.nextValue();

		// add repeating set_status
		std::string repeat_val = infile.nextValue();
		while (repeat_val != "") {
			evnt->components.push_back(Event_Component());
			e = &evnt->components.back();
			e->type = infile.key;
			e->s = repeat_val;

			repeat_val = infile.nextValue();
		}
	}
	else if (infile.key == "unset_status") {
		// @ATTR event.unset_status|string,...|Unsets specified statuses
		e->s = infile.nextValue();

		// add repeating unset_status
		std::string repeat_val = infile.nextValue();
		while (repeat_val != "") {
			evnt->components.push_back(Event_Component());
			e = &evnt->components.back();
			e->type = infile.key;
			e->s = repeat_val;

			repeat_val = infile.nextValue();
		}
	}
	else if (infile.key == "remove_item") {
		// @ATTR event.remove_item|integer,...|Removes specified itesm from hero inventory
		e->x = toInt(infile.nextValue());

		// add repeating remove_item
		std::string repeat_val = infile.nextValue();
		while (repeat_val != "") {
			evnt->components.push_back(Event_Component());
			e = &evnt->components.back();
			e->type = infile.key;
			e->x = toInt(repeat_val);

			repeat_val = infile.nextValue();
		}
	}
	else if (infile.key == "reward_xp") {
		// @ATTR event.reward_xp|integer|Reward hero with specified amount of experience points.
		e->x = toInt(infile.val);
	}
	else if (infile.key == "power") {
		// @ATTR event.power|power_id|Specify power coupled with event.
		e->x = toInt(infile.val);
	}
	else if (infile.key == "spawn") {
		// @ATTR event.spawn|[string,x(integer),y(integer)], ...|Spawn specified enemies at location
		e->s = infile.nextValue();
		e->x = toInt(infile.nextValue());
		e->y = toInt(infile.nextValue());

		// add repeating spawn
		std::string repeat_val = infile.nextValue();
		while (repeat_val != "") {
			evnt->components.push_back(Event_Component());
			e = &evnt->components.back();
			e->type = infile.key;

			e->s = repeat_val;
			e->x = toInt(infile.nextValue());
			e->y = toInt(infile.nextValue());

			repeat_val = infile.nextValue();
		}
	}
	else if (infile.key == "stash") {
		// @ATTR event.stash|string|
		e->s = infile.val;
	}
	else if (infile.key == "npc") {
		// @ATTR event.npc|string|
		e->s = infile.val;
	}
	else if (infile.key == "music") {
		// @ATTR event.music|string|Change background music to specified file.
		e->s = infile.val;
	}
	else if (infile.key == "cutscene") {
		// @ATTR event.cutscene|string|Show specified cutscene.
		e->s = infile.val;
	}
	else if (infile.key == "repeat") {
		// @ATTR event.repeat|string|
		e->s = infile.val;
	}
	else {
		fprintf(stderr, "EventManager: Unknown key value: %s in file %s in section %s\n", infile.key.c_str(), infile.getFileName().c_str(), infile.section.c_str());
	}
}