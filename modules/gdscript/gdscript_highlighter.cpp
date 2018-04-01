/*************************************************************************/
/*  gdscript_highlighter.cpp                                             */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2018 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2018 Godot Engine contributors (cf. AUTHORS.md)    */
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

#include "gdscript_highlighter.h"
#include "gdscript_tokenizer.h"
#include "scene/gui/text_edit.h"

inline bool _is_symbol(CharType c) {
	return is_symbol(c);
}

static bool _is_number(CharType c) {
	return (c >= '0' && c <= '9');
}

static bool _is_num_symbol(CharType c) {
	return ((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F') || c == '.' || c == 'e' || c == 'x' || c == '-');
}

Map<int, TextEdit::HighlighterInfo> GDSyntaxHighlighter::_get_line_syntax_highlighting(int p_line) {
	color_map.clear();
	last_token_col = 0;

	int in_region = -1;
	for (int i = 0; i < p_line; i++) {
		int ending_color_region = text_editor->_get_line_ending_color_region(i);
		if (in_region == -1) {
			in_region = ending_color_region;
		} else if (in_region == ending_color_region) {
			in_region = -1;
		} else {
			const Map<int, TextEdit::Text::ColorRegionInfo> &cri_map = text_editor->_get_line_color_region_info(i);
			for (const Map<int, TextEdit::Text::ColorRegionInfo>::Element *E = cri_map.front(); E; E = E->next()) {
				const TextEdit::Text::ColorRegionInfo &cri = E->get();
				if (cri.region == in_region) {
					in_region = -1;
				}
			}
		}
	}

	String line = text_editor->get_line(p_line);
	const Map<int, TextEdit::Text::ColorRegionInfo> &cri_map = text_editor->_get_line_color_region_info(p_line);

	/*
	 * If we start in a region move the tokenizer pass it.
	 */
	if (in_region > -1) {
		TextEdit::ColorRegion color_region = text_editor->_get_color_region(in_region);

		TextEdit::HighlighterInfo highlighter_info;
		highlighter_info.color = color_region.color;
		color_map[0] = highlighter_info;

		if (cri_map.size() <= 0) {
			return color_map;
		}

		int end_col = -1;
		for (Map<int, TextEdit::Text::ColorRegionInfo>::Element *E = cri_map.front(); E; E = E->next()) {
			if (E->get().region == in_region) {
				end_col = E->key();
				break;
			}
		}

		if (end_col == -1) {
			return color_map;
		}

		last_token_col = end_col + color_region.end_key.length();
		//		while (token && token != GDScriptTokenizer::TK_EOF && token != GDScriptTokenizer::TK_ERROR) {
		//			if (tokenizer.get_token_column() > last_token_col) {
		//				break;
		//			}
		//			tokenizer.advance();
		//			last_token = token;
		//			token = tokenizer.get_token();
		//		}
		line = line.substr(end_col + color_region.end_key.length(), line.length());
		in_region = -1;
	}

	GDScriptTokenizer::Token last_token;
	tokenizer.set_code(line);
	GDScriptTokenizer::Token token = tokenizer.get_token();

	while (token && token != GDScriptTokenizer::TK_EOF && token != GDScriptTokenizer::TK_ERROR) {
		Color color = font_color;
		int token_column = tokenizer.get_token_column();
		int overide_column = -1;

		int region_col = -1;
		if (in_region == -1) {
			if (cri_map.has(token_column)) {
				region_col = token_column;
			} else if (cri_map.has(token_column - 1)) {
				region_col = token_column - 1;
			} else if (cri_map.has(token_column + 1)) {
				region_col = token_column + 1;
			}
			in_region = (region_col > -1) ? cri_map[region_col].region : -1;
		}

		if (in_region > -1) {
			TextEdit::ColorRegion color_region = text_editor->_get_color_region(in_region);

			TextEdit::HighlighterInfo highlighter_info;
			highlighter_info.color = color_region.color;
			color_map[token_column] = highlighter_info;

			int end_col = -1;
			bool found = false;
			for (Map<int, TextEdit::Text::ColorRegionInfo>::Element *E = cri_map.front(); E; E = E->next()) {
				if (!found) {
					if (E->key() == region_col) {
						found = true;
					}
					continue;
				}
				if (E->get().region == in_region) {
					end_col = E->key();
					break;
				}
			}

			if (end_col == -1) {
				return color_map;
			}

			last_token_col = end_col + color_region.end_key.length();
			while (token && token != GDScriptTokenizer::TK_EOF && token != GDScriptTokenizer::TK_ERROR) {
				if (tokenizer.get_token_column() > last_token_col) {
					break;
				}
				tokenizer.advance();
				last_token = token;
				token = tokenizer.get_token();
			}

			in_region = -1;
			continue;
		}

		switch (token) {
			case GDScriptTokenizer::TK_OP_IN:
			case GDScriptTokenizer::TK_OP_NOT:
			case GDScriptTokenizer::TK_OP_OR:
			case GDScriptTokenizer::TK_OP_AND: {
				if (_is_symbol(line[token_column])) {
					color = symbol_color;
				} else {
					color = keyword_color;
				}
			} break;
			case GDScriptTokenizer::TK_PR_CLASS:
			case GDScriptTokenizer::TK_PR_EXTENDS:
			case GDScriptTokenizer::TK_PR_IS:
			case GDScriptTokenizer::TK_PR_ONREADY:
			case GDScriptTokenizer::TK_PR_TOOL:
			case GDScriptTokenizer::TK_PR_STATIC:
			case GDScriptTokenizer::TK_PR_EXPORT:
			case GDScriptTokenizer::TK_PR_SETGET:
			case GDScriptTokenizer::TK_PR_VAR:
			case GDScriptTokenizer::TK_PR_PRELOAD:
			case GDScriptTokenizer::TK_PR_ASSERT:
			case GDScriptTokenizer::TK_PR_YIELD:
			case GDScriptTokenizer::TK_PR_SIGNAL:
			case GDScriptTokenizer::TK_PR_BREAKPOINT:
			case GDScriptTokenizer::TK_PR_REMOTE:
			case GDScriptTokenizer::TK_PR_MASTER:
			case GDScriptTokenizer::TK_PR_SLAVE:
			case GDScriptTokenizer::TK_PR_SYNC:
			case GDScriptTokenizer::TK_PR_CONST:
			case GDScriptTokenizer::TK_PR_ENUM:
			case GDScriptTokenizer::TK_CF_IF:
			case GDScriptTokenizer::TK_CF_ELIF:
			case GDScriptTokenizer::TK_CF_ELSE:
			case GDScriptTokenizer::TK_CF_FOR:
			case GDScriptTokenizer::TK_CF_WHILE:
			case GDScriptTokenizer::TK_CF_DO:
			case GDScriptTokenizer::TK_CF_SWITCH:
			case GDScriptTokenizer::TK_CF_CASE:
			case GDScriptTokenizer::TK_CF_BREAK:
			case GDScriptTokenizer::TK_CF_CONTINUE:
			case GDScriptTokenizer::TK_CF_RETURN:
			case GDScriptTokenizer::TK_CF_MATCH:
			case GDScriptTokenizer::TK_CF_PASS:
			case GDScriptTokenizer::TK_SELF:
			case GDScriptTokenizer::TK_CONST_PI:
			case GDScriptTokenizer::TK_CONST_TAU:
			case GDScriptTokenizer::TK_CONST_INF:
			case GDScriptTokenizer::TK_CONST_NAN:
			case GDScriptTokenizer::TK_PR_FUNCTION: {
				color = keyword_color;
			} break;
			case GDScriptTokenizer::TK_CONSTANT: {
				// string and number token columns are at the end of the token so
				// we have to perform more work to get the start.
				// For strings we have to get the lengh and remove 2 for the quotes.
				// Hex and scientific notations are converted to their real values.
				// causing us to have to read the text.
				if (tokenizer.get_token_constant().is_num()) {
					color = number_color;

					int col = token_column - 2;
					while (col > -1) {
						if ((!_is_number(line[col]) && !_is_num_symbol(line[col])) || line[col] == ' ') {
							break;
						}
						col--;
					}
					overide_column = col + 1;
				} else if (tokenizer.get_token_constant().get_type() == Variant::STRING) {
					color = string_color;
					overide_column = (token_column - 2) - String(tokenizer.get_token_constant()).length();
				} else if (tokenizer.get_token_constant().get_type() == Variant::BOOL || tokenizer.get_token_constant().get_type() == Variant::NIL) {
					color = keyword_color;
				}
			} break;
			case GDScriptTokenizer::TK_IDENTIFIER: {
				if (last_token == GDScriptTokenizer::TK_PR_FUNCTION) {
					color = function_color;
				} else if (last_token == GDScriptTokenizer::TK_PERIOD) {
					color = member_color;
				} else if (last_token == GDScriptTokenizer::TK_DOLLAR) {
					color = get_node_shortcut_color;
					overide_column = last_token_col - 1;

					// get to the end of the node path
					tokenizer.advance();
					GDScriptTokenizer::Token node_token = tokenizer.get_token();
					while (node_token && (node_token == GDScriptTokenizer::TK_OP_DIV || node_token == GDScriptTokenizer::TK_IDENTIFIER)) {
						tokenizer.advance();
						node_token = tokenizer.get_token();
					}
				} else {
					String identifier = tokenizer.get_token_identifier();
					if (text_editor->has_keyword_color(identifier)) {
						color = text_editor->get_keyword_color(identifier);
					} else if (text_editor->has_member_color(identifier)) {
						color = text_editor->get_member_color(identifier);
					} else {
						color = font_color;
					}
				}
			} break;
			case GDScriptTokenizer::TK_BUILT_IN_FUNC: {
				color = keyword_color;
			} break;
			case GDScriptTokenizer::TK_BUILT_IN_TYPE: {
				color = built_in_type_color;
			} break;
			case GDScriptTokenizer::TK_BRACKET_OPEN:
			case GDScriptTokenizer::TK_BRACKET_CLOSE:
			case GDScriptTokenizer::TK_CURLY_BRACKET_OPEN:
			case GDScriptTokenizer::TK_CURLY_BRACKET_CLOSE:
			case GDScriptTokenizer::TK_PARENTHESIS_CLOSE:
			case GDScriptTokenizer::TK_COMMA:
			case GDScriptTokenizer::TK_SEMICOLON:
			case GDScriptTokenizer::TK_PERIOD:
			case GDScriptTokenizer::TK_QUESTION_MARK:
			case GDScriptTokenizer::TK_COLON:
			case GDScriptTokenizer::TK_DOLLAR:
			case GDScriptTokenizer::TK_OP_EQUAL:
			case GDScriptTokenizer::TK_OP_NOT_EQUAL:
			case GDScriptTokenizer::TK_OP_LESS:
			case GDScriptTokenizer::TK_OP_LESS_EQUAL:
			case GDScriptTokenizer::TK_OP_GREATER:
			case GDScriptTokenizer::TK_OP_ADD:
			case GDScriptTokenizer::TK_OP_SUB:
			case GDScriptTokenizer::TK_OP_MUL:
			case GDScriptTokenizer::TK_OP_DIV:
			case GDScriptTokenizer::TK_OP_MOD:
			case GDScriptTokenizer::TK_OP_SHIFT_LEFT:
			case GDScriptTokenizer::TK_OP_SHIFT_RIGHT:
			case GDScriptTokenizer::TK_OP_ASSIGN:
			case GDScriptTokenizer::TK_OP_ASSIGN_ADD:
			case GDScriptTokenizer::TK_OP_ASSIGN_SUB:
			case GDScriptTokenizer::TK_OP_ASSIGN_MUL:
			case GDScriptTokenizer::TK_OP_ASSIGN_DIV:
			case GDScriptTokenizer::TK_OP_ASSIGN_MOD:
			case GDScriptTokenizer::TK_OP_ASSIGN_SHIFT_LEFT:
			case GDScriptTokenizer::TK_OP_ASSIGN_SHIFT_RIGHT:
			case GDScriptTokenizer::TK_OP_ASSIGN_BIT_AND:
			case GDScriptTokenizer::TK_OP_ASSIGN_BIT_OR:
			case GDScriptTokenizer::TK_OP_ASSIGN_BIT_XOR:
			case GDScriptTokenizer::TK_OP_BIT_AND:
			case GDScriptTokenizer::TK_OP_BIT_OR:
			case GDScriptTokenizer::TK_OP_BIT_XOR:
			case GDScriptTokenizer::TK_OP_BIT_INVERT:
			case GDScriptTokenizer::TK_WILDCARD: {
				color = symbol_color;
			} break;
			case GDScriptTokenizer::TK_PARENTHESIS_OPEN: {
				if (last_token == GDScriptTokenizer::TK_IDENTIFIER) {
					_push_color(function_color, last_token_col - 1);
				}
				color = symbol_color;
			} break;
			default: {
			} break;
		}

		//print_line(itos(p_line) + " " + tokenizer.get_token_name(token));

		_push_color(color, overide_column);
		tokenizer.advance();
		last_token = token;
		token = tokenizer.get_token();
	}

	// check ending for comments
	bool comment = false;
	if (!line.empty()) {
		int token_column = tokenizer.get_token_column();
		if (token_column > 0) {
			//comment = (line[token_column - 1] == '#');
			if (!comment && token_column > 3 && token_column < line.size() - 3) {
				//comment = (line[token_column - 2] != '\\' && line[token_column - 1] == '"' && line[token_column] == '"' && line[token_column + 1] == '"');
			}
		}
	}

	//if (comment) {
	if (line.length() > tokenizer.get_token_column()) {
		_push_color(comment_color);
	}
	return color_map;
}

void GDSyntaxHighlighter::_push_color(Color p_color, int p_col) {
	if (p_col == -1) {
		p_col = tokenizer.get_token_column() - 1;
		last_token_col = p_col + 1;
	} else {
		last_token_col = p_col;
	}
	TextEdit::HighlighterInfo highlighter_info;
	highlighter_info.color = p_color;
	color_map[p_col] = highlighter_info;
}

String GDSyntaxHighlighter::get_name() {
	return "GDScript";
}

List<String> GDSyntaxHighlighter::get_supported_languages() {
	List<String> langauges;
	langauges.push_back("GDScript");
	return langauges;
}

void GDSyntaxHighlighter::_update_cache() {
	font_color = text_editor->get_color("font_color");
	comment_color = text_editor->get_color("comment_color");
	keyword_color = text_editor->get_color("keyword_color");
	symbol_color = text_editor->get_color("symbol_color");
	function_color = text_editor->get_color("function_color");
	string_color = text_editor->get_color("string_color");
	built_in_type_color = text_editor->get_color("built_in_type_color");
	number_color = text_editor->get_color("number_color");
	member_color = text_editor->get_color("member_variable_color");
	get_node_shortcut_color = text_editor->get_color("get_node_shortcut_color");
}

SyntaxHighlighter *GDSyntaxHighlighter::create() {
	return memnew(GDSyntaxHighlighter);
}
