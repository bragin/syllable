/*  libsyllable.so - the highlevel API library for Syllable
 *  Copyright (C) 2003 Syllable Team
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of version 2 of the GNU Library
 *  General Public License as published by the Free Software
 *  Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 *  MA 02111-1307, USA
 */

#ifndef	__F_UTIL_SHORTCUTKEY_H__
#define	__F_UTIL_SHORTCUTKEY_H__

#include <gui/guidefines.h>
#include <util/string.h>
#include <atheos/types.h>

namespace os {

class ShortcutKey {
	public:
	ShortcutKey( const char* pzKey, uint32 nQualifiers = 0 );
	ShortcutKey( const uint32 nKey, uint32 nQualifiers = 0 );
	ShortcutKey( const ShortcutKey& cShortcut );
	ShortcutKey();
	~ShortcutKey();

	void SetFromLabel( const String& cLabel );
	void SetFromLabel( const char* pzLabel );
	
	bool IsValid() const;

	bool operator<( const ShortcutKey& c ) const;
	bool operator==( const ShortcutKey& c ) const;
	
	private:
//	char*		m_pzKey;
	uint32		m_nKey;
	uint32		m_nQualifiers;

	uint32		m_nReserved1;
	uint32		m_nReserved2;

	void _SetKey( const char* pzKey );
};

} // end of namespace

#endif	// __F_UTIL_SHORTCUTKEY_H__

