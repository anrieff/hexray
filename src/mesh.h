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
#include "bbox.h"

struct KDTreeNode {
	Axis axis;
	double splitPos;
	union {
		KDTreeNode* children;
		std::vector<int>* triangles;
	};

	void initLeaf(const std::vector<int>& t_list)
	{
		axis = AXIS_NONE;
		this->triangles = new std::vector<int>(t_list);
	}

	void initBinaryNode(Axis axis, double sp)
	{
		this->axis = axis;
		splitPos = sp;
		children = new KDTreeNode[2];
	}

	~KDTreeNode()
	{
		if (axis == AXIS_NONE)
			delete triangles;
		else
			delete[] children;
	}
};

struct KDTreeStats {
	int numNodes;
	int numLeafNodes;
	int maxDepth;
	long long sumDepth;
	long long sumTriLeaf;

	void printStats();
};

class Mesh: public Geometry {
protected:
	std::vector<Vector> vertices;
	std::vector<Vector> normals;
	std::vector<Vector> uvs;
	std::vector<Triangle> triangles;
	BBox bbox;
	KDTreeNode* kdroot = nullptr;
	KDTreeStats kdstats;

	void computeBoundingGeometry();
	bool intersectTriangle(const Ray& ray, const Triangle& t, IntersectionInfo& info);
    void prepareTriangles();

	void buildKD(KDTreeNode* node, BBox bbox, const std::vector<int>& t_list, int depth);
	bool intersectKD(KDTreeNode* node, const BBox& bbox, const Ray& ray, IntersectionInfo& info);
public:
	bool faceted = false;
	bool backfaceCulling = false;
	bool useKDTree = true;
	bool autoSmooth = false;
	bool recenter = false;

	~Mesh();

	bool loadFromOBJ(const char* filename);

	void baseProperties(ParsedBlock& pb)
	{
		pb.getBoolProp("faceted", &faceted);
		pb.getBoolProp("backfaceCulling", &backfaceCulling);
		pb.getBoolProp("useKDTree", &useKDTree);
		pb.getBoolProp("autoSmooth", &autoSmooth);
		pb.getBoolProp("recenter", &recenter);
	}

	void fillProperties(ParsedBlock& pb)
	{
		char fn[256];
		baseProperties(pb);
		if (pb.getFilenameProp("file", fn)) {
			if (!loadFromOBJ(fn)) {
				pb.signalError("Could not parse OBJ file!");
			}
		} else {
			pb.requiredProp("file");
		}
	}

	void beginRender();

	virtual bool intersect(const Ray& ray, IntersectionInfo& info) override;
};
