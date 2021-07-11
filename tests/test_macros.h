/*************************************************************************/
/*  test_macros.h                                                        */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2021 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2021 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#ifndef TEST_MACROS_H
#define TEST_MACROS_H

#include "core/templates/map.h"
#include "core/variant/variant.h"

// See documentation for doctest at:
// https://github.com/onqtam/doctest/blob/master/doc/markdown/readme.md#reference
#include "thirdparty/doctest/doctest.h"

// The test is skipped with this, run pending tests with `--test --no-skip`.
#define TEST_CASE_PENDING(name) TEST_CASE(name *doctest::skip())

// The test case is marked as failed, but does not fail the entire test run.
#define TEST_CASE_MAY_FAIL(name) TEST_CASE(name *doctest::may_fail())

// Provide aliases to conform with Godot naming conventions (see error macros).
#define TEST_COND(cond, ...) DOCTEST_CHECK_FALSE_MESSAGE(cond, __VA_ARGS__)
#define TEST_FAIL(cond, ...) DOCTEST_FAIL(cond, __VA_ARGS__)
#define TEST_FAIL_COND(cond, ...) DOCTEST_REQUIRE_FALSE_MESSAGE(cond, __VA_ARGS__)
#define TEST_FAIL_COND_WARN(cond, ...) DOCTEST_WARN_FALSE_MESSAGE(cond, __VA_ARGS__)

// Temporarily disable error prints to test failure paths.
// This allows to avoid polluting the test summary with error messages.
// The `_print_error_enabled` boolean is defined in `core/print_string.cpp` and
// works at global scope. It's used by various loggers in `should_log()` method,
// which are used by error macros which call into `OS::print_error`, effectively
// disabling any error messages to be printed from the engine side (not tests).
#define ERR_PRINT_OFF _print_error_enabled = false;
#define ERR_PRINT_ON _print_error_enabled = true;

// Stringify all `Variant` compatible types for doctest output by default.
// https://github.com/onqtam/doctest/blob/master/doc/markdown/stringification.md

#define DOCTEST_STRINGIFY_VARIANT(m_type)                        \
	template <>                                                  \
	struct doctest::StringMaker<m_type> {                        \
		static doctest::String convert(const m_type &p_val) {    \
			const Variant val = p_val;                           \
			return val.get_construct_string().utf8().get_data(); \
		}                                                        \
	};

#define DOCTEST_STRINGIFY_VARIANT_POINTER(m_type)                \
	template <>                                                  \
	struct doctest::StringMaker<m_type> {                        \
		static doctest::String convert(const m_type *p_val) {    \
			const Variant val = p_val;                           \
			return val.get_construct_string().utf8().get_data(); \
		}                                                        \
	};

DOCTEST_STRINGIFY_VARIANT(Variant);
DOCTEST_STRINGIFY_VARIANT(::String); // Disambiguate from `doctest::String`.

DOCTEST_STRINGIFY_VARIANT(Vector2);
DOCTEST_STRINGIFY_VARIANT(Vector2i);
DOCTEST_STRINGIFY_VARIANT(Rect2);
DOCTEST_STRINGIFY_VARIANT(Rect2i);
DOCTEST_STRINGIFY_VARIANT(Vector3);
DOCTEST_STRINGIFY_VARIANT(Vector3i);
DOCTEST_STRINGIFY_VARIANT(Transform2D);
DOCTEST_STRINGIFY_VARIANT(Plane);
DOCTEST_STRINGIFY_VARIANT(Quaternion);
DOCTEST_STRINGIFY_VARIANT(AABB);
DOCTEST_STRINGIFY_VARIANT(Basis);
DOCTEST_STRINGIFY_VARIANT(Transform3D);

DOCTEST_STRINGIFY_VARIANT(::Color); // Disambiguate from `doctest::Color`.
DOCTEST_STRINGIFY_VARIANT(StringName);
DOCTEST_STRINGIFY_VARIANT(NodePath);
DOCTEST_STRINGIFY_VARIANT(RID);
DOCTEST_STRINGIFY_VARIANT_POINTER(Object);
DOCTEST_STRINGIFY_VARIANT(Callable);
DOCTEST_STRINGIFY_VARIANT(Signal);
DOCTEST_STRINGIFY_VARIANT(Dictionary);
DOCTEST_STRINGIFY_VARIANT(Array);

DOCTEST_STRINGIFY_VARIANT(PackedByteArray);
DOCTEST_STRINGIFY_VARIANT(PackedInt32Array);
DOCTEST_STRINGIFY_VARIANT(PackedInt64Array);
DOCTEST_STRINGIFY_VARIANT(PackedFloat32Array);
DOCTEST_STRINGIFY_VARIANT(PackedFloat64Array);
DOCTEST_STRINGIFY_VARIANT(PackedStringArray);
DOCTEST_STRINGIFY_VARIANT(PackedVector2Array);
DOCTEST_STRINGIFY_VARIANT(PackedVector3Array);
DOCTEST_STRINGIFY_VARIANT(PackedColorArray);

// Register test commands to be launched from the command-line.
// For instance: REGISTER_TEST_COMMAND("gdscript-parser" &test_parser_func).
// Example usage: `godot --test gdscript-parser`.

typedef void (*TestFunc)();
extern Map<String, TestFunc> *test_commands;
int register_test_command(String p_command, TestFunc p_function);

#define REGISTER_TEST_COMMAND(m_command, m_function)                    \
	DOCTEST_GLOBAL_NO_WARNINGS(DOCTEST_ANONYMOUS(_DOCTEST_ANON_VAR_)) = \
			register_test_command(m_command, m_function);               \
	DOCTEST_GLOBAL_NO_WARNINGS_END()

// Utility macro to send an action event to a given object
// Requires Message Queue and InputMap to be setup.

#define SEND_GUI_ACTION(m_object, m_action)                                                           \
	{                                                                                                 \
		const List<Ref<InputEvent>> *events = InputMap::get_singleton()->action_get_events(m_action); \
		const List<Ref<InputEvent>>::Element *first_event = events->front();                          \
		Ref<InputEventKey> event = first_event->get();                                                \
		event->set_pressed(true);                                                                     \
		m_object->call("_gui_input", event);                                                          \
		MessageQueue::get_singleton()->flush();                                                       \
	}

// Utility class / macros for testing signals
//
// Use SIGNAL_WATCH(*object, "signal_name") to start watching
// Makes sure to call SIGNAL_UNWATCH(*object, "signal_name") to stop watching in cleanup, this is not done automaticaly.
//
// The SignalWatcher will capture all signals and their args sent between checks.
//
// Use SIGNAL_CHECK("signal_name"), Vector<Vector<Variant>>), to check the arguments of all fireed signals.
// The outer vector is each fired signal, the inner vector the list of arguements for that signal. Order does matter.
//
// Use SIGNAL_CHECK_FALSE("signal_name") to check if a signal was not fired.
//
// Use SIGNAL_DISCARD("signal_name") to discard records all of the given signal, use only in placed you don't need to check.
//
// All signals are automaticaly discared between test/sub test cases.

#include "core/object/callable_method_pointer.h"
#include "core/object/object.h"

class SignalWatcher : public Object {
private:
	inline static SignalWatcher *singleton;

	/* Equal to: Map<String, Vector<Vector<Variant>>> */
	Map<String, Array> _signals;
	void _add_signal_entry(const Array &p_args, const String &p_name) {
		if (!_signals.has(p_name)) {
			_signals[p_name] = Array();
		}
		_signals[p_name].push_back(p_args);
	}

	void _signal_callback(Variant p_arg1, const String &p_name) {
		Array args;
		args.push_back(p_arg1);
		_add_signal_entry(args, p_name);
	}

public:
	static SignalWatcher *get_singleton() { return singleton; }

	void watch_signal(Object *p_object, const String &p_name) {
		Vector<Variant> args;
		args.push_back(p_name);
		p_object->connect(p_name, callable_mp(this, &SignalWatcher::_signal_callback), args);
	}

	void unwatch_signal(Object *p_object, const String &p_name) {
		p_object->disconnect(p_name, callable_mp(this, &SignalWatcher::_signal_callback));
	}

	bool check(const String &p_name, const Array &p_args) {
		if (!_signals.has(p_name)) {
			MESSAGE("Signal ", p_name, " not emitted");
			return false;
		}

		if (p_args.size() != _signals[p_name].size()) {
			MESSAGE("Signal has " << _signals[p_name] << " expected " << p_args);
			discard_signal(p_name);
			return false;
		}

		bool match = true;
		for (int i = 0; i < p_args.size(); i++) {
			if (((Array)p_args[i]).size() != ((Array)_signals[p_name][i]).size()) {
				MESSAGE("Signal has " << _signals[p_name][i] << " expected " << p_args[i]);
				match = false;
				continue;
			}

			for (int j = 0; j < ((Array)p_args[i]).size(); j++) {
				if (((Array)p_args[i])[j] != ((Array)_signals[p_name][i])[j]) {
					MESSAGE("Signal has " << _signals[p_name][i] << " expected " << p_args[i]);
					match = false;
					break;
				}
			}
		}

		discard_signal(p_name);
		return match;
	}

	bool check_false(const String &p_name) {
		bool has = _signals.has(p_name);
		discard_signal(p_name);
		return !has;
	}

	void discard_signal(const String &p_name) {
		if (_signals.has(p_name)) {
			_signals.erase(p_name);
		}
	}

	void _clear_signals() {
		_signals.clear();
	}

	SignalWatcher() {
		singleton = this;
	}
};

#define SIGNAL_WATCH(m_object, m_signal) SignalWatcher::get_singleton()->watch_signal(m_object, m_signal);
#define SIGNAL_UNWATCH(m_object, m_signal) SignalWatcher::get_singleton()->unwatch_signal(m_object, m_signal);

#define SIGNAL_CHECK(m_signal, m_args) CHECK(SignalWatcher::get_singleton()->check(m_signal, m_args));
#define SIGNAL_CHECK_FALSE(m_signal) CHECK(SignalWatcher::get_singleton()->check_false(m_signal));
#define SIGNAL_DISCARD(m_signal) SignalWatcher::get_singleton()->discard_signal(m_signal);

#endif // TEST_MACROS_H
