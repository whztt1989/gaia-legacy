/* 
 * Copyright (C) 2006-2008 Dmitry Marakasov
 *
 * This file is part of Gaia.
 *
 * Gaia is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Gaia is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Gaia.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GAIA__EARTHVIEW_H__
#define __GAIA__EARTHVIEW_H__

#include <vector>

#include "Viewpoint.h"
#include "Layer.h"

#define NAV_DRAG_PAN	0x01
#define NAV_DRAG_ZOOM	0x02

#define NAV_PAN_LEFT	0x01
#define NAV_PAN_RIGHT	0x02
#define NAV_PAN_UP	0x04
#define NAV_PAN_DOWN	0x08

#define NAV_ZOOM_IN	0x10
#define NAV_ZOOM_OUT	0x20

namespace gaia {

/**
 * Abstract class for earth view.
 */
class EarthView {
public:
	EarthView(EarthView *ancestor = 0);
	virtual ~EarthView();

	int ActivateLayer(LayerMeta *meta);
	int DeactivateLayer(LayerMeta *meta);

	virtual void Render() = 0;
	virtual void Animate(double delta) = 0;

	void Resize(int width, int height);

	virtual int StartDrag(int x, int y, int flags);
	virtual int Drag(int fromx, int fromy, int x, int y, int flags);
	virtual int Click(int x, int y, int flags);

	virtual int StartMovement(int flags);
	virtual int StopMovement(int flags);
	virtual int SingleMovement(int flags);

	void MoveToPosition(double x, double y);

protected:
	std::vector<Layer*>	m_Layers;	///< Layers to use

	Eye		m_Eye;			///< Current viewer's position

	int		m_ViewportWidth;	///< Viewport width in pixels
	int		m_ViewportHeight;	///< Viewport height in pixels
};

} /* namespace gaia */

#endif