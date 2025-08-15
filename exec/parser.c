#include "parser.h"
#include <assert.h>
#include <stdio.h>

static Node *parse_expression(ParserContext *ctx, Env *env);
static Node *parse_list(ParserContext *ctx, Env *env);
static Node *parse_atom(ParserContext *ctx, Env *env);
static char *parse_undefined_symbol(ParserContext *ctx);
static Node *parse_ifexpr(ParserContext *ctx, Env *env);
static Node *parse_def(ParserContext *ctx, Env *env);
static Node *parse_let(ParserContext *ctx, Env *env);
static Node *parse_function(ParserContext *ctx, Env *env);
static Node *parse_quote(ParserContext *ctx, Env *env);
static void synchronize(ParserContext *ctx);
static void skip_whitespace_and_comments(struct ParserContext *ctx);

typedef Node *(*SpecialFormParser)(ParserContext *ctx, Env *env);

static const struct
{
	const char *name;
	SpecialFormParser parser;
} special_forms[] = {
	{"if", parse_ifexpr},	{"def", parse_def},
	{"let", parse_let},		{"lambda", parse_function},
	{"quote", parse_quote},
};

static SpecialFormParser find_special_form_parser(const char *name)
{
	for (size_t i = 0;
		 i < sizeof(special_forms) / sizeof(special_forms[0]); ++i)
	{
		if (strcmp(name, special_forms[i].name) == 0)
		{
			return special_forms[i].parser;
		}
	}
	return NULL;
}

static ParserError *parser_error_create(Token *trouble_token,
										const char *error_msg,
										enum ParserErrorType type)
{
	ParserError *error = malloc(sizeof(ParserError));
	error->token = token_copy(trouble_token);
	error->type = type;
	error->error_msg = strdup(error_msg);
	return error;
}

static void parser_error_cleanup(ParserError *error)
{
	if (!error)
	{
		return;
	}
	token_cleanup(&error->token);
	free(error->error_msg);
	free(error);
}

static void parser_register_error(ParserContext *ctx,
								  ParserError *error)
{
	g_ptr_array_add(ctx->errors, error);
}

static void error_at_current_token(ParserContext *ctx,
								   const char *error_msg)
{
	if (ctx->panic_mode)
	{
		return;
	}
	ctx->panic_mode = true;
	ParserError *e = parser_error_create(&ctx->current_token,
										 error_msg, PARSER_ERROR);
	parser_register_error(ctx, e);
}

static void warning_at_current_token(ParserContext *ctx,
									 const char *warning_msg)
{
	if (ctx->panic_mode)
	{
		return;
	}
	ParserError *e = parser_error_create(&ctx->current_token,
										 warning_msg, PARSER_WARNING);
	parser_register_error(ctx, e);
}

static void parser_error_cleanup_v(void *error)
{
	parser_error_cleanup((ParserError *)error);
}

static void advance(ParserContext *ctx)
{
	token_cleanup(&ctx->current_token);
	ctx->current_token = lexer_next(ctx->lexer);
}

static void skip_whitespace_and_comments(struct ParserContext *ctx)
{
	while (ctx->current_token.type == TOKEN_WHITESPACE ||
		   ctx->current_token.type == TOKEN_COMMENT)
	{
		advance(ctx);
	}
}

static bool consume(ParserContext *ctx,
					enum TokenType expected_type,
					const char *msg_on_failure)
{
	skip_whitespace_and_comments(ctx);
	if (ctx->current_token.type == expected_type)
	{
		advance(ctx);
		return true;
	}

	error_at_current_token(ctx, msg_on_failure);
	return false;
}

static Node *parse_ifexpr(ParserContext *ctx, Env *env)
{
	Node *condition = parse_expression(ctx, env);
	if (condition == NULL)
		return NULL;
	skip_whitespace_and_comments(ctx);

	Node *then_branch = parse_expression(ctx, env);
	if (then_branch == NULL)
	{
		node_free(condition);
		return NULL;
	}

	Node *else_branch = NULL;
	skip_whitespace_and_comments(ctx);
	if (ctx->current_token.type != TOKEN_RPAREN)
	{
		else_branch = parse_expression(ctx, env);
		if (else_branch == NULL)
		{
			node_free(condition);
			node_free(then_branch);
			return NULL;
		}
	}

	skip_whitespace_and_comments(ctx);
	if (ctx->current_token.type != TOKEN_RPAREN)
	{
		error_at_current_token(
			ctx, "Too many arguments for 'if' expression.");
		node_free(condition);
		node_free(then_branch);
		if (else_branch)
			node_free(else_branch);
		return NULL;
	}

	return node_create_if_expr(condition, then_branch, else_branch);
}

static Node *parse_function(ParserContext *ctx, Env *env)
{
	if (!consume(ctx, TOKEN_LPAREN,
				 "Expected '(' for function parameter list."))
	{
		return NULL;
	}

	Env *body_env = env_create(env);
	StringArray *params = string_array_new();

	skip_whitespace_and_comments(ctx);
	while (ctx->current_token.type == TOKEN_SYMBOL)
	{
		char *param_name = parse_undefined_symbol(ctx);
		if (param_name == NULL)
		{
			string_array_free(params);
			env_cleanup(body_env);
			return NULL;
		}
		string_array_add(params, param_name);
		env_emplace(body_env, param_name, get_placeholder());
		free(param_name);
		skip_whitespace_and_comments(ctx);
	}

	if (!consume(ctx, TOKEN_RPAREN,
				 "Expected ')' to close parameter list."))
	{
		string_array_free(params);
		env_cleanup(body_env);
		return NULL;
	}

	for (guint i = 0; i < params->_array->len; i++)
	{
		char *param_name = string_array_index(params, i);
		env_emplace(body_env, param_name, get_placeholder());
	}

	NodeArray *body_expressions = node_array_new();

	skip_whitespace_and_comments(ctx);
	while (ctx->current_token.type != TOKEN_RPAREN)
	{
		Node *expr = parse_expression(ctx, body_env);
		if (expr)
		{
			node_array_add(body_expressions, expr);
		}
		else
		{
			error_at_current_token(
				ctx, "Failed to parse expression in function body.");
			string_array_free(params);
			node_array_free(body_expressions);
			env_cleanup(body_env);
			return NULL;
		}
		skip_whitespace_and_comments(ctx);
	}

	if (body_expressions->_array->len == 0)
	{
		error_at_current_token(ctx, "Function body cannot be empty.");
		string_array_free(params);
		node_array_copy(body_expressions);
		env_cleanup(body_env);
		return NULL;
	}

	return node_create_function(params, body_expressions, env);
}
static Node *parse_def_variable(ParserContext *ctx, Env *env)
{
	char *name = strdup(ctx->current_token.lexeme);
	advance(ctx);

	Node *value = parse_expression(ctx, env);
	if (value == NULL)
	{
		free(name);
		return NULL;
	}

	skip_whitespace_and_comments(ctx);
	if (ctx->current_token.type != TOKEN_RPAREN)
	{
		error_at_current_token(ctx, "Too many arguments for 'def'.");
		free(name);
		node_free(value);
		return NULL;
	}

	if (env_lookup(env, name) != NULL)
	{
		char *warning_msg;
		asprintf(&warning_msg, "Redefinition of variable '%s'", name);
		assert(warning_msg && "Out of memory");
		warning_at_current_token(ctx, warning_msg);
		free(warning_msg);
	}

	env_emplace(env, name, value);
	VarBinding *binding = var_binding_create(name, value);
	Node *def_node = node_create_def(binding);
	free(name);
	node_free(value);
	return def_node;
}

static Node *parse_def_function(ParserContext *ctx, Env *env)
{
	consume(ctx, TOKEN_LPAREN,
			"Expected '(' after def for function signature.");

	char *name = parse_undefined_symbol(ctx);
	if (name == NULL)
		return NULL;

	StringArray *params = string_array_new();
	skip_whitespace_and_comments(ctx);
	while (ctx->current_token.type == TOKEN_SYMBOL)
	{
		char *param_name = parse_undefined_symbol(ctx);
		if (param_name == NULL)
		{
			string_array_free(params);
			free(name);
			return NULL;
		}
		string_array_add(params, param_name);
		free(param_name);
		skip_whitespace_and_comments(ctx);
	}

	if (!consume(ctx, TOKEN_RPAREN,
				 "Expected ')' to close parameter list."))
	{
		string_array_free(params);
		free(name);
		return NULL;
	}

	env_emplace(env, name, get_placeholder());
	Env *body_env = env_create(env);
	for (guint i = 0; i < params->_array->len; i++)
	{
		env_emplace(body_env, string_array_index(params, i),
					get_placeholder());
	}

	NodeArray *body_expressions = node_array_new();

	skip_whitespace_and_comments(ctx);
	while (ctx->current_token.type != TOKEN_RPAREN)
	{
		Node *expr = parse_expression(ctx, body_env);
		if (expr)
		{
			node_array_add(body_expressions, expr);
		}
		else
		{
			string_array_free(params);
			node_array_free(body_expressions);
			env_cleanup(body_env);
			free(name);
			return NULL;
		}
		skip_whitespace_and_comments(ctx);
	}

	Node *function_node =
		node_create_function(params, body_expressions, env);

	// env_emplace(body_env, name, function_node);
	// env_emplace(env, name, function_node);

	VarBinding *binding = var_binding_create(name, function_node);
	Node *def_node = node_create_def(binding);
	free(name);
	node_free(function_node);
	return def_node;
}

static Node *parse_def(ParserContext *ctx, Env *env)
{
	skip_whitespace_and_comments(ctx);

	if (ctx->current_token.type == TOKEN_SYMBOL)
	{
		return parse_def_variable(ctx, env);
	}
	else if (ctx->current_token.type == TOKEN_LPAREN)
	{
		return parse_def_function(ctx, env);
	}
	else
	{
		error_at_current_token(
			ctx, "Expected a symbol or a list after 'def'.");
		return NULL;
	}
}

static char *parse_undefined_symbol(ParserContext *ctx)
{
	skip_whitespace_and_comments(ctx);
	if (ctx->current_token.type != TOKEN_SYMBOL)
	{
		error_at_current_token(ctx, "Expected a symbol.");
		return NULL;
	}
	char *name = strdup(ctx->current_token.lexeme);
	advance(ctx);
	return name;
}

static Node *parse_let(ParserContext *ctx, Env *env)
{
	if (!consume(ctx, TOKEN_LPAREN, "Expected '(' for let-bindings."))
	{
		return NULL;
	}

	Env *let_env = env_create(env);
	VarBindingArray *bindings = var_binding_array_new();

	skip_whitespace_and_comments(ctx);
	while (ctx->current_token.type != TOKEN_RPAREN)
	{
		if (!consume(ctx, TOKEN_LPAREN,
					 "Expected '(' for a binding pair."))
		{
			var_binding_array_free(bindings);
			env_cleanup(let_env);
			return NULL;
		}

		skip_whitespace_and_comments(ctx);
		if (ctx->current_token.type != TOKEN_SYMBOL)
		{
			error_at_current_token(
				ctx, "Expected a symbol for binding name.");
			var_binding_array_free(bindings);
			env_cleanup(let_env);
			return NULL;
		}
		char *name = strdup(ctx->current_token.lexeme);
		advance(ctx);

		Node *value = parse_expression(ctx, env);
		if (value == NULL)
		{
			free(name);
			var_binding_array_free(bindings);
			env_cleanup(let_env);
			return NULL;
		}

		env_emplace(let_env, name, value);

		if (!consume(ctx, TOKEN_RPAREN,
					 "Expected ')' to close binding pair."))
		{
			free(name);
			node_free(value);
			var_binding_array_free(bindings);
			env_cleanup(let_env);
			return NULL;
		}

		VarBinding *binding = var_binding_create(name, value);
		var_binding_array_add(bindings, binding);
		env_emplace(let_env, name, value);
		free(name);
		node_free(value);
		skip_whitespace_and_comments(ctx);
	}
	advance(ctx);

	NodeArray *body_expressions = node_array_new();

	skip_whitespace_and_comments(ctx);
	while (ctx->current_token.type != TOKEN_RPAREN)
	{
		Node *expr = parse_expression(ctx, let_env);
		if (expr)
		{
			node_array_add(body_expressions, expr);
		}
		else
		{
			error_at_current_token(
				ctx, "Failed to parse expression in let body.");
			var_binding_array_free(bindings);
			node_array_free(body_expressions);
			env_cleanup(let_env);
			return NULL;
		}
		skip_whitespace_and_comments(ctx);
	}

	if (body_expressions->_array->len == 0)
	{
		error_at_current_token(ctx, "Let body cannot be empty.");
		var_binding_array_free(bindings);
		node_array_free(body_expressions);
		env_cleanup(let_env);
		return NULL;
	}

	return node_create_let(bindings, body_expressions, let_env);
}

static Node *parse_quote(ParserContext *ctx, Env *env)
{
	Node *quoted_expr = parse_expression(ctx, env);
	if (quoted_expr == NULL)
		return NULL;
	return node_create_quote(quoted_expr);
}

static Node *parse_call(ParserContext *ctx, Node *callable, Env *env)
{
	NodeArray *args = node_array_new();

	skip_whitespace_and_comments(ctx);
	while (ctx->current_token.type != TOKEN_RPAREN &&
		   ctx->current_token.type != TOKEN_EOF)
	{
		Node *arg = parse_expression(ctx, env);
		if (arg == NULL)
		{
			node_array_free(args);
			node_free(callable);
			return NULL;
		}
		node_array_add(args, arg);
		skip_whitespace_and_comments(ctx);
	}
	return node_create_function_call(callable, args);
}

static Node *parse_expression(ParserContext *ctx, Env *env)
{
	skip_whitespace_and_comments(ctx);
	switch (ctx->current_token.type)
	{
	case TOKEN_LPAREN:
		return parse_list(ctx, env);
	case TOKEN_QUOTE:
		advance(ctx);
		return parse_quote(ctx, env);
	case TOKEN_SYMBOL:
	case TOKEN_NUMBER:
	case TOKEN_STRING:
		return parse_atom(ctx, env);
	case TOKEN_EOF:
		return NULL;
	case TOKEN_RPAREN:
		error_at_current_token(ctx, "Unexpected ')'");
		return NULL;
	case TOKEN_ERROR:
		error_at_current_token(ctx, ctx->current_token.lexeme);
		return NULL;
	default:
		error_at_current_token(ctx, "Unexpected token");
		return NULL;
	}
}

static Node *parse_literal_number(Token *token)
{
	if (strchr(token->lexeme, '.') != NULL)
	{
		return node_create_literal_float(strtod(token->lexeme, NULL));
	}
	else
	{
		return node_create_literal_int(
			strtol(token->lexeme, NULL, 10));
	}
}

static Node *
parse_literal_symbol(ParserContext *ctx, Token *token, Env *env)
{
	if (strcmp(token->lexeme, "#t") == 0)
	{
		return node_create_literal_bool(true);
	}
	else if (strcmp(token->lexeme, "#f") == 0)
	{
		return node_create_literal_bool(false);
	}
	else
	{
		if (env_lookup(env, token->lexeme) == NULL)
		{
			char *error_msg;
			asprintf(&error_msg, "Undefined variable: '%s'",
					 token->lexeme);
			ParserError *e =
				parser_error_create(token, error_msg, PARSER_ERROR);
			parser_register_error(ctx, e);
			return NULL;
		}
		return node_create_variable(token->lexeme, env);
	}
}

static Node *parse_atom(ParserContext *ctx, Env *env)
{
	const Token *token = &ctx->current_token;
	Node *res = NULL;
	switch (token->type)
	{
	case TOKEN_SYMBOL:
		res = parse_literal_symbol(ctx, &ctx->current_token, env);
		break;
	case TOKEN_NUMBER:
		res = parse_literal_number(&ctx->current_token);
		break;
	case TOKEN_STRING:
		res = node_create_literal_string(token->lexeme);
		break;
	default:
		error_at_current_token(ctx, "Unrecognized atom type");
	}
	if (res)
	{
		advance(ctx);
	}
	return res;
}

static void synchronize(ParserContext *ctx)
{
	ctx->panic_mode = false;
	while (ctx->current_token.type != TOKEN_EOF)
	{
		switch (ctx->current_token.type)
		{
		case TOKEN_LPAREN:
			return;
		case TOKEN_SYMBOL:
			if (strcmp(ctx->current_token.lexeme, "def") == 0 ||
				strcmp(ctx->current_token.lexeme, "let") == 0)
			default:
				break;
		}
		advance(ctx);
	}
}

static Node *parse_list(ParserContext *ctx, Env *env)
{
	consume(ctx, TOKEN_LPAREN, "");

	skip_whitespace_and_comments(ctx);
	if (ctx->current_token.type == TOKEN_RPAREN)
	{
		advance(ctx);
		return node_create_literal_bool(false);
	}

	Node *first_expr = parse_expression(ctx, env);
	if (first_expr == NULL)
	{
		return NULL;
	}

	Node *result_node = NULL;
	SpecialFormParser special_parser = NULL;
	if (first_expr->type == NODE_VARIABLE &&
		(special_parser = find_special_form_parser(
			 first_expr->variable.name)) != NULL)
	{
		node_free(first_expr);
		result_node = special_parser(ctx, env);
	}
	else
	{
		result_node = parse_call(ctx, first_expr, env);
	}

	skip_whitespace_and_comments(ctx);
	if (ctx->current_token.type != TOKEN_RPAREN)
	{
		error_at_current_token(ctx,
							   "Expected ')' to close the list.");
		if (result_node)
			node_free(result_node);
		return NULL;
	}
	consume(ctx, TOKEN_RPAREN, "");

	return result_node;
}

static void populate_global_env(Env *env)
{
	char *special_forms[] = {"+",	   "-",	 "/",	"*",	"=",
							 "<",	   ">",	 ">=",	"<=",	"let",
							 "lambda", "if", "def", "quote"};
	int num_elements =
		sizeof(special_forms) / sizeof(special_forms[0]);
	for (int i = 0; i < num_elements; i++)
	{
		env_emplace(env, special_forms[i], get_placeholder());
	}
}

ParserContext *parser_create(char *source_code)
{
	ParserContext *ctx = malloc(sizeof(ParserContext));
	assert(ctx && "Out of memory");

	ctx->lexer = lexer_create(source_code);
	ctx->global_env = env_create(NULL);
	populate_global_env(ctx->global_env);
	ctx->panic_mode = false;
	ctx->errors =
		g_ptr_array_new_with_free_func(parser_error_cleanup_v);

	Location starting_location = {0, 0, 0, 0};
	Token starting_token =
		token_create(TOKEN_WHITESPACE, " ", starting_location);
	ctx->current_token = starting_token;
	advance(ctx);

	return ctx;
}

void parser_cleanup(ParserContext *ctx)
{
	if (!ctx)
	{
		return;
	}
	token_cleanup(&ctx->current_token);
	g_ptr_array_free(ctx->errors, TRUE);
	env_cleanup(ctx->global_env);
	lexer_cleanup(ctx->lexer);
	free(ctx);
}

NodeArray *parser_parse(ParserContext *ctx)
{
	NodeArray *node_array = node_array_new();
	while (ctx->current_token.type != TOKEN_EOF)
	{
		Node *n = parse_expression(ctx, ctx->global_env);
		if (n)
		{
			node_array_add(node_array, n);
		}
		else if (ctx->current_token.type == TOKEN_EOF)
		{
			return node_array;
		}
		else
		{
			synchronize(ctx);
		}
	}
	return node_array;
}

void print_source_line(const char *source_code, int line_number)
{
	const char *line_start = source_code;
	int current_line = 1;

	while (current_line < line_number && *line_start != '\0')
	{
		if (*line_start == '\n')
		{
			current_line++;
		}
		line_start++;
	}

	if (current_line == line_number)
	{
		const char *p = line_start;
		while (*p != '\0' && *p != '\n')
		{
			putchar(*p);
			p++;
		}
		putchar('\n');
	}
}

static inline int max(const int a, const int b) //
{
	return a > b ? a : b;
}

static inline const char *
match_error_type_string(enum ParserErrorType t)
{
	switch (t)
	{
	case PARSER_ERROR:
		return "Error";
	case PARSER_WARNING:
		return "Warning";
	}
}

void print_error(const ParserError *e, const char *source_code)
{
	const char *underlines =
		"^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^"
		"^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^"
		"^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^";
	const char *error_type_str = match_error_type_string(e->type);
	Location loc = e->token.location;
	int len = max(loc.end.col - loc.start.col, 1);

	printf("%s [%d,%d]: %s\n", error_type_str, loc.start.line,
		   loc.start.col, e->error_msg);
	print_source_line(source_code, loc.start.line);
	printf("%*s%.*s\n\n", loc.start.col - 1, "", len, underlines);
}

void parser_print_errors(ParserContext *ctx)
{
	for (guint i = 0; i < ctx->errors->len; i++)
	{
		const ParserError *e = g_ptr_array_index(ctx->errors, i);
		print_error(e, ctx->lexer->buffer.data);
	}
}