/**
 * @file type-query.c
 *
 * Provides a way for setools to make queries about types and
 * attributes within a policy.  The caller obtains a query object,
 * fills in its parameters, and then runs the query; it obtains a
 * vector of results.  Searches are conjunctive -- all fields of the
 * search query must match for a datum to be added to the results
 * query.
 *
 * @author Kevin Carr  kcarr@tresys.com
 * @author Jeremy A. Mowery jmowery@tresys.com
 * @author Jason Tang  jtang@tresys.com
 *
 * Copyright (C) 2006 Tresys Technology, LLC
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "policy-query.h"

struct apol_type_query {
	char *type_name;
	unsigned int flags;
	regex_t *regex;
};

struct apol_attr_query {
	char *attr_name;
	unsigned int flags;
	regex_t *regex;
};

/******************** type queries ********************/

int apol_get_type_by_query(apol_policy_t *p,
			   apol_type_query_t *t,
			   apol_vector_t **v)
{
	qpol_iterator_t *iter;
	int retval = -1;
	*v = NULL;
	if (qpol_policy_get_type_iter(p->qh, p->p, &iter) < 0) {
		return -1;
	}
	if ((*v = apol_vector_create()) == NULL) {
		ERR(p, "Out of memory!");
		goto cleanup;
	}
	for ( ; !qpol_iterator_end(iter); qpol_iterator_next(iter)) {
		qpol_type_t *type;
		unsigned char isattr, isalias;
		if (qpol_iterator_get_item(iter, (void **) &type) < 0) {
			goto cleanup;
		}
		if (qpol_type_get_isattr(p->qh, p->p, type, &isattr) < 0 ||
		    qpol_type_get_isalias(p->qh, p->p, type, &isalias) < 0) {
			goto cleanup;
		}
		if (isattr || isalias) {
			continue;
		}
		if (t != NULL) {
			int compval = apol_compare_type(p,
							type, t->type_name,
							t->flags, &(t->regex));
			if (compval < 0) {
				goto cleanup;
			}
			else if (compval == 0) {
				continue;
			}
		}
		if (apol_vector_append(*v, type)) {
			ERR(p, "Out of memory!");
			goto cleanup;
		}
	}

	retval = 0;
 cleanup:
	if (retval != 0) {
		apol_vector_destroy(v, NULL);
	}
	qpol_iterator_destroy(&iter);
	return retval;
}

apol_type_query_t *apol_type_query_create(void)
{
	return calloc(1, sizeof(apol_type_query_t));
}

void apol_type_query_destroy(apol_type_query_t **t)
{
	if (*t != NULL) {
		free((*t)->type_name);
		apol_regex_destroy(&(*t)->regex);
		free(*t);
		*t = NULL;
	}
}

int apol_type_query_set_type(apol_policy_t *p, apol_type_query_t *t, const char *name)
{
	return apol_query_set(p, &t->type_name, &t->regex, name);
}

int apol_type_query_set_regex(apol_policy_t *p, apol_type_query_t *t, int is_regex)
{
	return apol_query_set_regex(p, &t->flags, is_regex);
}

/******************** attribute queries ********************/

int apol_get_attr_by_query(apol_policy_t *p,
			   apol_attr_query_t *a,
			   apol_vector_t **v)
{
	qpol_iterator_t *iter;
	int retval = -1;
	*v = NULL;
	if (qpol_policy_get_type_iter(p->qh, p->p, &iter) < 0) {
		return -1;
	}
	if ((*v = apol_vector_create()) == NULL) {
		ERR(p, "Out of memory!");
		goto cleanup;
	}
	for ( ; !qpol_iterator_end(iter); qpol_iterator_next(iter)) {
		qpol_type_t *type;
		unsigned char isattr, isalias;
		if (qpol_iterator_get_item(iter, (void **) &type) < 0) {
			goto cleanup;
		}
		if (qpol_type_get_isattr(p->qh, p->p, type, &isattr) < 0 ||
		    qpol_type_get_isalias(p->qh, p->p, type, &isalias) < 0) {
			goto cleanup;
		}
		if (!isattr || isalias) {
			continue;
		}
		if (a != NULL) {
			char *attr_name;
			int compval;
			if (qpol_type_get_name(p->qh, p->p, type, &attr_name) < 0) {
				goto cleanup;
			}
			compval = apol_compare(p, attr_name, a->attr_name,
					       a->flags, &(a->regex));
			if (compval < 0) {
				goto cleanup;
			}
			else if (compval == 0) {
				continue;
			}
		}
		if (apol_vector_append(*v, type)) {
			ERR(p, "Out of memory!");
			goto cleanup;
		}
	}

	retval = 0;
 cleanup:
	if (retval != 0) {
		apol_vector_destroy(v, NULL);
	}
	qpol_iterator_destroy(&iter);
	return retval;
}

apol_attr_query_t *apol_attr_query_create(void)
{
	return calloc(1, sizeof(apol_attr_query_t));
}

void apol_attr_query_destroy(apol_attr_query_t **a)
{
	if (*a != NULL) {
		free((*a)->attr_name);
		apol_regex_destroy(&(*a)->regex);
		free(*a);
		*a = NULL;
	}
}

int apol_attr_query_set_attr(apol_policy_t *p, apol_attr_query_t *a, const char *name)
{
	return apol_query_set(p, &a->attr_name, &a->regex, name);
}

int apol_attr_query_set_regex(apol_policy_t *p, apol_attr_query_t *a, int is_regex)
{
	return apol_query_set_regex(p, &a->flags, is_regex);
}
