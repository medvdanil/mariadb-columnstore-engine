/* c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil
 * vi: set shiftwidth=4 tabstop=4 expandtab:
 *  :indentSize=4:tabSize=4:noTabs=true:
 *
 * Copyright (C) 2016 MariaDB Corporaton
 *
 * This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2 of
 * the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#include <stdint.h>
#include <memory.h>

template <Size>
class BigDecimal {
private:
	uint8_t data[Size];
	BigDecimal() {
		memset(data, 0xFF, sizeof(uint8_t) * Size);
		res.data[0] = 0xFE;
	}
	BigDecimal(uint64_t val) {
		memset(data, 0, sizeof(uint8_t) * Size);
		((uint64_t*)data[0]) = val;
	}
	static BigDecimal getEmptyRowVal() {
		BigDecimal res;
		res.data[0] = 0xFF;
		return res;		
	}
};
