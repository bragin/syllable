/* Look up a symbol in the loaded objects.
   Copyright (C) 1995, 1996, 1997, 1998, 1999 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#if VERSIONED
# define FCT do_lookup_versioned
# define ARG const struct r_found_version *const version,
#else
# define FCT do_lookup
# define ARG
#endif

/* Inner part of the lookup functions.  We return a value > 0 if we
   found the symbol, the value 0 if nothing is found and < 0 if
   something bad happened.  */
static inline int
FCT (const char *undef_name, unsigned long int hash,
     const ElfW(Sym) *ref, struct sym_val *result,
     struct r_scope_elem *scope, size_t i, const char *reference_name,
     ARG struct link_map *skip, int reloc_type)
{
  struct link_map **list = scope->r_list;
  size_t n = scope->r_nlist;
  struct link_map *map;

  do
    {
      const ElfW(Sym) *symtab;
      const char *strtab;
      const ElfW(Half) *verstab;
      ElfW(Symndx) symidx;
      const ElfW(Sym) *sym;
#if ! VERSIONED
      int num_versions = 0;
      const ElfW(Sym) *versioned_sym = NULL;
#endif

      map = list[i];

      /* Here come the extra test needed for `_dl_lookup_symbol_skip'.  */
      if (skip != NULL && map == skip)
	continue;

      /* Don't search the executable when resolving a copy reloc.  */
      if (elf_machine_lookup_noexec_p (reloc_type)
	  && map->l_type == lt_executable)
	continue;

      /* Print some debugging info if wanted.  */
      if (_dl_debug_symbols)
	_dl_debug_message (1, "symbol=", undef_name, ";  lookup in file=",
			   map->l_name[0] ? map->l_name : _dl_argv[0],
			   "\n", NULL);

      symtab = (const void *) map->l_info[DT_SYMTAB]->d_un.d_ptr;
      strtab = (const void *) map->l_info[DT_STRTAB]->d_un.d_ptr;
      verstab = map->l_versyms;

      /* Search the appropriate hash bucket in this object's symbol table
	 for a definition for the same symbol name.  */
      for (symidx = map->l_buckets[hash % map->l_nbuckets];
	   symidx != STN_UNDEF;
	   symidx = map->l_chain[symidx])
	{
	  sym = &symtab[symidx];

	  if (sym->st_value == 0 || /* No value.  */
	      (elf_machine_lookup_noplt_p (reloc_type) /* Reject PLT entry.  */
	       && sym->st_shndx == SHN_UNDEF))
	    continue;

	  if (ELFW(ST_TYPE) (sym->st_info) > STT_FUNC)
	    /* Ignore all but STT_NOTYPE, STT_OBJECT and STT_FUNC entries
	       since these are no code/data definitions.  */
	    continue;

	  if (sym != ref && strcmp (strtab + sym->st_name, undef_name))
	    /* Not the symbol we are looking for.  */
	    continue;

#if VERSIONED
	  if (verstab == NULL)
	    {
	      /* We need a versioned symbol but haven't found any.  If
		 this is the object which is referenced in the verneed
		 entry it is a bug in the library since a symbol must
		 not simply disappear.  */
	      if (version->filename != NULL
		  && _dl_name_match_p (version->filename, map))
		return -2;
	      /* Otherwise we accept the symbol.  */
	    }
	  else
	    {
	      /* We can match the version information or use the
		 default one if it is not hidden.  */
	      ElfW(Half) ndx = verstab[symidx] & 0x7fff;
	      if ((map->l_versions[ndx].hash != version->hash
		   || strcmp (map->l_versions[ndx].name, version->name))
		  && (version->hidden || map->l_versions[ndx].hash
		      || (verstab[symidx] & 0x8000)))
		/* It's not the version we want.  */
		continue;
	    }
#else
	  /* No specific version is selected.  When the object file
	     also does not define a version we have a match.
	     Otherwise we accept the default version, or in case there
	     is only one version defined, this one version.  */
	  if (verstab != NULL)
	    {
	      ElfW(Half) ndx = verstab[symidx] & 0x7fff;
	      if (ndx > 2) /* map->l_versions[ndx].hash != 0) */
		{
		  /* Don't accept hidden symbols.  */
		  if ((verstab[symidx] & 0x8000) == 0 && num_versions++ == 0)
		    /* No version so far.  */
		    versioned_sym = sym;
		  continue;
		}
	    }
#endif

	  /* There cannot be another entry for this symbol so stop here.  */
	  goto found_it;
	}

      /* If we have seen exactly one versioned symbol while we are
	 looking for an unversioned symbol and the version is not the
	 default version we still accept this symbol since there are
	 no possible ambiguities.  */
#if VERSIONED
	sym = NULL;
#else
	sym = num_versions == 1 ? versioned_sym : NULL;
#endif

      if (sym != NULL)
	{
	found_it:
	  switch (ELFW(ST_BIND) (sym->st_info))
	    {
	    case STB_GLOBAL:
	      /* Global definition.  Just what we need.  */
	      result->s = sym;
	      result->m = map;
	      return 1;
	    case STB_WEAK:
	      /* Weak definition.  Use this value if we don't find another.  */
	      if (! result->s)
		{
		  result->s = sym;
		  result->m = map;
		}
	      break;
	    default:
	      /* Local symbols are ignored.  */
	      break;
	    }
	}

#if VERSIONED
      /* If this current map is the one mentioned in the verneed entry
	 and we have not found a weak entry, it is a bug.  */
      if (symidx == STN_UNDEF && version->filename != NULL
	  && _dl_name_match_p (version->filename, map))
	return -1;
#endif
    }
  while (++i < n);

  /* We have not found anything until now.  */
  return 0;
}

#undef FCT
#undef ARG
#undef VERSIONED
