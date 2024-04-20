/***************************************************************************
 *   Copyright (C) 2009-2024 by Veselin Georgiev, Slavomir Kaslev,         *
 *                              Deyan Hadzhiev et al                       *
 *   admin@raytracing-bg.net                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
/**
 * @File mesh.h
 * @Brief Contains the Mesh class.
 */
#pragma once

#include <vector>
#include "geometry.h"
#include "vector.h"

/// A structure to represent a single triangle in the mesh
struct Triangle {
	int v[3]; //!< holds indices to the three vertices of the triangle (indexes in the `vertices' array in the Mesh)
	int n[3]; //!< holds indices to the three normals of the triangle (indexes in the `normals' array)
	int t[3]; //!< holds indices to the three texture coordinates of the triangle (indexes in the `uvs' array)
	Vector gnormal; //!< The geometric normal of the mesh (AB ^ AC, normalized)
	Vector AB, AC, ABcrossAC; //!< precomputed AB, AC and AB^AC
	Vector dNdx, dNdy;
};
// the C vertex of triangle with index 5 is:
// mesh.vertices[mesh.triangles[5].v[2]];

class Mesh: public Geometry {
protected:
	std::vector<Vector> vertices;
	std::vector<Vector> normals;
	std::vector<Vector> uvs;
	std::vector<Triangle> triangles;
	Sphere boundingSphere;

	void computeBoundingGeometry();
	bool intersectTriangle(const Ray& ray, const Triangle& t, IntersectionInfo& info);
    void prepareTriangles();
public:
	bool faceted = false;
	bool backfaceCulling = false;

	bool loadFromOBJ(const char* filename);

	void beginRender();

	virtual bool intersect(Ray ray, IntersectionInfo& info) override;
};
