/***************************************************************************
  qgstessellator.h
  --------------------------------------
  Date                 : July 2017
  Copyright            : (C) 2017 by Martin Dobias
  Email                : wonder dot sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSTESSELLATOR_H
#define QGSTESSELLATOR_H

#include "qgis_core.h"
#include "qgis_sip.h"
#include "qgsrectangle.h"

class QgsPolygon;
class QgsMultiPolygon;
class QgsLineString;

#include <QVector>
#include <QVector4D>
#include <QtMath>
#include <array>
#include <functional>
#include <memory>

/**
 * \ingroup core
 * \brief Tessellates polygons into triangles.
 *
 * It is expected that client code will create the tessellator object, then repeatedly call
 * addPolygon() method that will generate triangles, and finally call data() to get final vertex data.
 *
 * Optionally provides extrusion by adding triangles that serve as walls when extrusion height is non-zero.
 *
 */
class CORE_EXPORT QgsTessellator
{
  public:
    QgsTessellator();

    /**
     * \brief Creates tessellator with a specified origin point of the world (in map coordinates)
     * \deprecated QGIS 4.0. Use the default QgsTessellator() constructor and individual setters instead.
     */
    Q_DECL_DEPRECATED QgsTessellator(
      double originX, double originY, bool addNormals, bool invertNormals = false, bool addBackFaces = false, bool noZ = false, bool addTextureCoords = false, int facade = 3, float textureRotation = 0.0f
    ) SIP_DEPRECATED;

    /**
     * Creates tessellator with a specified \a bounds of input geometry coordinates.
     * This constructor allows the tessellator to map input coordinates to a desirable range for numerical
     * stability during calculations.
     *
     * If \a noZ is TRUE, then a 2-dimensional tessellation only will be performed and all z coordinates will be ignored.
     *
     * \since QGIS 3.10
     * \deprecated QGIS 4.0. Use the default QgsTessellator() constructor and individual setters instead.
     */
    Q_DECL_DEPRECATED QgsTessellator(
      const QgsRectangle &bounds, bool addNormals, bool invertNormals = false, bool addBackFaces = false, bool noZ = false, bool addTextureCoords = false, int facade = 3, float textureRotation = 0.0f
    ) SIP_DEPRECATED;

    /**
     * Sets the origin point of the map.
     * \since QGIS 4.0
     */
    void setOrigin( const QgsVector3D &origin );

    /**
     * Returns the origin point of the map.
     * \since QGIS 4.0
     */
    QgsVector3D origin() const { return mOrigin; }

    /**
     * Sets scaling and the bounds of the input geometry coordinates.
     * \since QGIS 4.0
     */
    void setBounds( const QgsRectangle &bounds );

    /**
     * Sets whether Z values from the input geometries are ignored (TRUE) or not (FALSE).
     * By default, this is FALSE.
     * \since QGIS 4.0
     */
    void setInputZValueIgnored( bool ignore );

    /**
     * Returns whether Z values from the input geometries are ignored (TRUE) or not (FALSE).
     * \since QGIS 4.0
     */
    bool isZValueIgnored() const { return mInputZValueIgnored; }

    /**
     * Sets which faces should be generated during extrusion.
     * \since QGIS 4.0
     */
    void setExtrusionFaces( Qgis::ExtrusionFaces faces );

    /**
     * Returns which faces are generated during extrusion.
     * \since QGIS 4.0
     */
    Qgis::ExtrusionFaces extrusionFaces() const { return mExtrusionFaces; }

    /**
     * Sets the ellipsoid of the globe that polygons are being tessellated onto, switching the
     * tessellator into globe (geocentric) mode. Set \a semiMajorAxis to 0 (the default) for flat
     * scenes.
     *
     * In globe mode input coordinates are treated as geocentric (ECEF) positions, and:
     *
     * - the triangles a polygon is tessellated into are recursively subdivided until they follow
     *   the curvature of the globe, instead of cutting through it on the single flat plane the
     *   polygon was triangulated in
     * - vertex normals are the outward ellipsoid normal at each vertex, rather than the normal of
     *   that flat plane
     * - extrusion height is applied along those outward normals instead of along the world Z axis,
     *   which is only a meaningful "up" direction for flat scenes, so extruded walls lean outwards
     *
     * \note Only the triangles are subdivided, so the walls of an extruded polygon still follow the
     * straight chords between the input ring's vertices. They meet the roof at those vertices, but
     * a ring segment spanning a large angle will leave the roof bulging out above its wall.
     *
     * \since QGIS 4.3
     */
    void setGlobeEllipsoid( double semiMajorAxis, double semiMinorAxis );

    /**
     * Sets the rotation of texture UV coordinates (in degrees).
     * \deprecated QGIS 4.2. Handled in shaders now. No longer has any effect on the texture coordinates.
     */
    Q_DECL_DEPRECATED void setTextureRotation( float rotation ) SIP_DEPRECATED;

    /**
     * Returns the rotation of texture UV coordinates (in degrees).
     * \deprecated QGIS 4.2. Handled in shaders now. No longer has any effect on the texture coordinates.
     */
    Q_DECL_DEPRECATED float textureRotation() const SIP_DEPRECATED { return 0; }

    /**
     * Sets whether texture UV coordinates should be added to the output data.
     *
     * \see hasTextureUVs()
     * \since QGIS 4.0
     */
    void setAddTextureUVs( bool addTextureUVs );

    /**
     * Returns TRUE if texture UV coordinates are being added to the output data.
     *
     * \see setAddTextureUVs()
     * \since QGIS 4.0
     */
    bool hasTextureUVs() const { return mAddTextureCoords; }

    /**
     * Sets whether normals should be added to the output data.
     *
     * \see hasNormals()
     * \since QGIS 4.0
     */
    void setAddNormals( bool addNormals );

    /**
     * Returns TRUE if normals are being added to the output data.
     *
     * \see setAddNormals()
     * \since QGIS 4.0
     */
    bool hasNormals() const { return mAddNormals; }

    /**
     * Sets whether tangents should be added to the output data.
     *
     * \see hasTangents()
     * \since QGIS 4.2
     */
    void setAddTangents( bool addTangents );

    /**
     * Returns TRUE if tangents are being added to the output data.
     *
     * \see setAddTangents()
     * \since QGIS 4.2
     */
    bool hasTangents() const { return mAddTangents; }

    /**
     * Sets whether back faces should be added to the output data.
     *
     * \see hasBackFacesEnabled()
     * \since QGIS 4.0
     */
    void setBackFacesEnabled( bool addBackFaces );

    /**
     * Returns TRUE if back faces are being added to the output data.
     *
     * \see setBackFacesEnabled()
     * \since QGIS 4.0
     */
    bool hasBackFacesEnabled() const { return mAddBackFaces; }

    /**
     * Sets whether normals should be inverted.
     *
     * \see hasInvertedNormals()
     * \since QGIS 4.0
     */
    void setInvertNormals( bool invertNormals );

    /**
     * Returns TRUE if normals are inverted.
     *
     * \see setInvertNormals()
     * \since QGIS 4.0
     */
    bool hasInvertedNormals() const { return mInvertNormals; }

    /**
     * Sets the triangulation algorithm.
     * \since QGIS 4.0
     */
    void setTriangulationAlgorithm( Qgis::TriangulationAlgorithm algorithm );

    /**
     * Returns the algorithm used for triangulation.
     * \since QGIS 4.0
     */
    Qgis::TriangulationAlgorithm triangulationAlgorithm() const { return mTriangulationAlgorithm; }

    /**
     * Sets whether the "up" direction should be the Z axis on output (TRUE),
     * otherwise the "up" direction will be the Y axis (FALSE). The default
     * value is FALSE (to keep compatibility for existing tessellator use cases).
     * \deprecated QGIS 4.2. Has no effect no, outputs are always z-up.
     */
    Q_DECL_DEPRECATED void setOutputZUp( bool zUp ) SIP_DEPRECATED;

    /**
     * Returns whether the "up" direction should be the Z axis on output (TRUE),
     * otherwise the "up" direction will be the Y axis (FALSE). The default
     * value is FALSE (to keep compatibility for existing tessellator use cases).
     * \deprecated QGIS 4.2. Has no effect no, outputs are always z-up.
     */
    Q_DECL_DEPRECATED bool isOutputZUp() const SIP_DEPRECATED { return true; }

    //! Tessellates a triangle and adds its vertex entries to the output data array
    void addPolygon( const QgsPolygon &polygon, float extrusionHeight );

    /**
     * Returns array of triangle vertex data
     *
     * Vertice coordinates are stored as (x, z, -y)
     *
     * \deprecated QGIS 4.0. Use vertexBuffer() in combination with indexBuffer().
     */
    Q_DECL_DEPRECATED QVector<float> data() const SIP_DEPRECATED;

    /**
     * Returns index buffer for the generated points.
     * \since QGIS 4.0
     */
    QByteArray indexBuffer() const;

    /**
     * Returns vertex buffer for the generated points.
     * \since QGIS 4.0
     */
    QByteArray vertexBuffer() const;

    //! Returns the number of vertices stored in the output data array
    int dataVerticesCount() const;

    //! Returns size of one vertex entry in bytes
    int stride() const { return mStride; }

    /**
     * Returns size of one index entry in bytes
     * \since QGIS 4.0
    */
    int indexStride() const { return sizeof( uint32_t ); }

    /**
     * Returns the triangulation as a multipolygon geometry.
     */
    std::unique_ptr< QgsMultiPolygon > asMultiPolygon() const SIP_SKIP;

    /**
     * Returns minimal Z value of the data (in world coordinates)
     * \since QGIS 3.12
     */
    float zMinimum() const { return mZMin; }

    /**
     * Returns maximal Z value of the data (in world coordinates)
     * \since QGIS 3.12
     */
    float zMaximum() const { return mZMax; }

    /**
     * Returns a descriptive error string if the tessellation failed.
     *
     * \since QGIS 3.34
     */
    QString error() const { return mError; }

    /**
     * Returns unique vertex count.
     * \since QGIS 4.0
     */
    int uniqueVertexCount() const;

  private:
    struct VertexPoint
    {
        QVector3D position;
        QVector3D normal;
        QVector4D tangent;

        inline bool operator==( const VertexPoint &other ) const { return position == other.position && normal == other.normal && tangent == other.tangent; }
    };

    //! Receives a triangle's three corners along with the surface normal at each of them
    using TriangleEmitter = std::function<void( const std::array<QVector3D, 3> &points, const std::array<QVector3D, 3> &normals )>;

    friend size_t qHash( const VertexPoint &key, size_t seed ) { return qHashMulti( seed, key.position.x(), key.position.y(), key.position.z() ); }

    QVector<uint32_t> mIndexBuffer;

    void updateStride();
    void setExtrusionFacesLegacy( int facade );
    void calculateBaseTransform( const QVector3D &pNormal, QMatrix4x4 *base ) const;
    QVector3D applyTransformWithExtrusion( const QVector3D point, const QVector3D &normal, float extrusionHeight, QMatrix4x4 *transformMatrix, const QgsPoint *originOffset );

    //! Returns TRUE if polygons are being tessellated onto a globe, in geocentric coordinates
    bool isGlobe() const { return mGlobeSemiMajorAxis > 0 && !mInputZValueIgnored; }
    /**
     * Returns how far \a p sits from the globe's centre, as a multiple of the ellipsoid's own radius
     * in that same direction: 1 for a point exactly on the ellipsoid, 2 for one twice as far out.
     * Dividing a point by this value therefore drops it onto the ellipsoid surface, along the ray
     * from the geocentre.
     */
    double globeSurfaceRatio( const QgsVector3D &p ) const;

    /**
     * Subdivides the flat triangles of \a trianglePoints (in triangulation base space) into ones
     * small enough to follow the globe's curvature, and hands each of them, together with the globe
     * normal at each of its three corners, to \a emitTriangle. The corners passed to the callback
     * are already in world space, relative to \a baseOrigin, so they need no further transform.
     */
    void subdivideTrianglesForGlobe( const std::vector<QVector3D> &trianglePoints, const QMatrix4x4 &baseToWorld, const QgsPoint &baseOrigin, const TriangleEmitter &emitTriangle );

    /**
     * Writes the roof and floor faces of a single triangle, with a \a normals entry per corner, to
     * the output buffers, including the back faces of each if mAddBackFaces is set. Which of the two
     * faces are written, and how far the roof is extruded, come from mBuildRoof, mBuildFloor and
     * mExtrusionHeight, all set by addPolygon() for the polygon being processed.
     *
     * If \a vertexBuffer is NULLPTR the corners are appended without any vertex sharing, otherwise
     * vertices already present in it are reused, with \a vertexBufferOffset being the index its
     * first vertex was written at.
     */
    void addTriangle(
      const std::array<QVector3D, 3> &points, const std::array<QVector3D, 3> &normals, QMatrix4x4 *transformMatrix, const QgsPoint *originOffset, QHash<VertexPoint, unsigned int> *vertexBuffer, size_t vertexBufferOffset
    );
    void addVertex(
      const QVector3D &point,
      const QVector3D &normal,
      const QVector4D &tangent,
      float extrusionHeight,
      QMatrix4x4 *transformMatrix,
      const QgsPoint *originOffset,
      QHash<VertexPoint, unsigned int> *vertexBuffer,
      const size_t &vertexBufferOffset,
      bool isFloor = false
    );
    void addVertex( const QVector3D &point, const QVector3D &normal, const QVector4D &tangent, float extrusionHeight, QMatrix4x4 *transformMatrix, const QgsPoint *originOffset, bool isFloor = false );
    void makeWalls( const QgsLineString &ring, bool ccw, float extrusionHeight );
    void addExtrusionWallQuad( const QVector3D &pt1, const QVector3D &pt2, float height, float u1, float u2, const QVector3D &normal1, const QVector3D &normal2 );
    std::vector<QVector3D> generateConstrainedDelaunayTriangles( const QgsPolygon *polygonNew );
    std::vector<QVector3D> generateEarcutTriangles( const QgsPolygon *polygonNew );

    QgsVector3D mOrigin = QgsVector3D( 0, 0, 0 );
    bool mAddNormals = false;
    bool mAddTangents = false;
    bool mInvertNormals = false;
    bool mAddBackFaces = false;
    bool mAddTextureCoords = false;
    QVector<float> mData;
    int mStride = 3 * sizeof( float );
    bool mInputZValueIgnored = false;
    Qgis::ExtrusionFaces mExtrusionFaces = Qgis::ExtrusionFace::Walls | Qgis::ExtrusionFace::Roof;
    double mGlobeSemiMajorAxis = 0;
    double mGlobeSemiMinorAxis = 0;
    Qgis::TriangulationAlgorithm mTriangulationAlgorithm = Qgis::TriangulationAlgorithm::ConstrainedDelaunay;
    float mScale = 1.0f;
    QString mError;

    // set by addPolygon() for the polygon it is working on, and read by addTriangle()
    float mExtrusionHeight = 0;
    bool mBuildRoof = false;
    bool mBuildFloor = false;

    float mZMin = std::numeric_limits<float>::max();
    float mZMax = -std::numeric_limits<float>::max();

    // one degree of angular resolution when splitting a triangle across the globe
    double mGlobeSubdivisionGranularity = M_PI / 180.0;

    // tangents of the flat roof and floor faces, and of the back face of each
    QVector4D mRoofTangent { 1.0f, 0.0f, 0.0f, 1.0f };
    QVector4D mRoofBackTangent { 1.0f, 0.0f, 0.0f, -1.0f };
    QVector4D mFloorTangent { -1.0f, 0.0f, 0.0f, 1.0f };
    QVector4D mFloorBackTangent { -1.0f, 0.0f, 0.0f, -1.0f };
};


#endif // QGSTESSELLATOR_H
