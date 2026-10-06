/***************************************************************************
  qgsfeature3dhandler_p.cpp
  --------------------------------------
  Date                 : January 2019
  Copyright            : (C) 2019 by Martin Dobias
  Email                : wonder dot sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include "qgsfeature3dhandler_p.h"

#include "qgs3drendercontext.h"
#include "qgscoordinatereferencesystem.h"
#include "qgsgeometry.h"

#include <QVector>
#include <QVector3D>

/// @cond PRIVATE

void QgsFeature3DHandler::updateZRangeFromPositions( const QVector<QVector3D> &positions )
{
  for ( const QVector3D &pos : positions )
  {
    if ( pos.z() < mZMin )
      mZMin = pos.z();
    if ( pos.z() > mZMax )
      mZMax = pos.z();
  }
}

bool QgsFeature3DHandler::clipGeometryIfTooLarge( QgsGeometry &geom, const Qgs3DRenderContext &context ) const
{
  // GEOS clips in 2D XY, but on a globe the chunk's XY rectangle is not a region on the
  // ellipsoid, and dropping/interpolating Z along an ECEF chord moves vertices off the surface
  if ( context.crs().type() == Qgis::CrsType::Geocentric )
    return false;

  // let's clip gigantic geometries to the chunk's extents
  const QgsRectangle bbox = geom.boundingBox();
  if ( bbox.width() > MAX_GEOM_BBOX_SIZE || bbox.height() > MAX_GEOM_BBOX_SIZE )
  {
    geom = geom.clipped( mChunkExtent.toRectangle() );
    return true;
  }
  return false;
}

/// @endcond
