#include <iostream>
#include "scene.h"
#include <cstring>
#include <span>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/string_cast.hpp>
#include "tiny_obj_loader.h"

#define OCTREE 1

Scene::Scene(string filename) {
    cout << "Reading scene from " << filename << " ..." << endl;
    cout << " " << endl;

    char* fname = (char*)filename.c_str();
    fp_in.open(fname);
    if (!fp_in.is_open()) {
        cout << "Error reading from file - aborting!" << endl;
        throw;
    }
    while (fp_in.good()) {
        string line;
        utilityCore::safeGetline(fp_in, line);
        if (!line.empty()) {
            vector<string> tokens = utilityCore::tokenizeString(line);
            cout << tokens[0] << endl;
            if (strcmp(tokens[0].c_str(), "MATERIAL") == 0) {
                loadMaterial(tokens[1]);
                cout << " " << endl;
            }
            else if (strcmp(tokens[0].c_str(), "OBJECTTXT") == 0) {
                loadGeom(tokens[1]);
                cout << " " << endl;
            }
            else if (strcmp(tokens[0].c_str(), "OBJECTOBJ") == 0) {
                loadGeomFromGLTF(tokens[1]);
                cout << " " << endl;
            }
            else if (strcmp(tokens[0].c_str(), "CAMERA") == 0) {
                loadCamera();
                cout << " " << endl;
            }
        }
    }

}

union convertInt
{
    int a;
    unsigned char b[0];
};

union convertFloat
{
    float a;
    unsigned char b[0];
};

unsigned short getUnsignedShort(std::vector<unsigned char>& data, int start) {
    convertInt tmp;
    tmp.b[0] = data[start];
    tmp.b[1] = data[start + 1];
    return tmp.a;
}

glm::vec3 getVec3(std::vector<unsigned char>& data, int start) {
    convertFloat byteToFloat;

    float x, y, z;
    byteToFloat.b[0] = data[start];
    byteToFloat.b[1] = data[start + 1];
    byteToFloat.b[2] = data[start + 2];
    byteToFloat.b[3] = data[start + 3];

    x = byteToFloat.a;
    byteToFloat.b[0] = data[start + 4];
    byteToFloat.b[1] = data[start + 5];
    byteToFloat.b[2] = data[start + 6];
    byteToFloat.b[3] = data[start + 7];
    y = byteToFloat.a;

    byteToFloat.b[0] = data[start + 8];
    byteToFloat.b[1] = data[start + 9];
    byteToFloat.b[2] = data[start + 10];
    byteToFloat.b[3] = data[start + 11];
    z = byteToFloat.a;

    return glm::vec3(x, y, z);
}

int Scene::loadGeomFromGLTF(string objectid) {
    int id = atoi(objectid.c_str());
    if (id != geoms.size()) {
        cout << "ERROR: OBJECT ID does not match expected number of geoms" << endl;
        return -1;
    }
    else {
        cout << "Loading Geom " << id << "..." << endl;


        std::string line;
        utilityCore::safeGetline(fp_in, line);

        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;

        std::string warn;
        std::string err;

        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, line.c_str());

        if (!warn.empty()) {
            std::cout << warn << std::endl;
        }

        if (!err.empty()) {
            std::cerr << err << std::endl;
        }

        if (!ret) {
            exit(1);
        }


        Geom newGeom;
        newGeom.type = MESH;
        newGeom.startTriangleIndex = triangles.size();

        // Loop over shapes
        for (size_t s = 0; s < shapes.size(); s++) {
            // Loop over faces(polygon)
            size_t index_offset = 0;
            for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
                int fv = shapes[s].mesh.num_face_vertices[f];
                Triangle tri;
                // Loop over vertices in the face.
                for (size_t v = 0; v < fv; v++) {
                    // access to vertex
                    tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                    tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
                    tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
                    tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];
                    newGeom.bboxMax.x = max(newGeom.bboxMax.x, vx);
                    newGeom.bboxMax.y = max(newGeom.bboxMax.y, vy);
                    newGeom.bboxMax.z = max(newGeom.bboxMax.z, vz);

                    newGeom.bboxMin.x = min(newGeom.bboxMin.x, vx);
                    newGeom.bboxMin.y = min(newGeom.bboxMin.y, vy);
                    newGeom.bboxMin.z = min(newGeom.bboxMin.z, vz);
                    //tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
                    //tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
                    //tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];

                    //tinyobj::real_t tx = attrib.texcoords[2 * idx.texcoord_index + 0];
                    //tinyobj::real_t ty = attrib.texcoords[2 * idx.texcoord_index + 1];

                    // Optional: vertex colors
                    // tinyobj::real_t red = attrib.colors[3*idx.vertex_index+0];
                    // tinyobj::real_t green = attrib.colors[3*idx.vertex_index+1];
                    // tinyobj::real_t blue = attrib.colors[3*idx.vertex_index+2];
                    tri.vert[v] = glm::vec3(vx, vy, vz);
                    tri.uv[v] = glm::vec2(0, 1);
                }
                index_offset += fv;
                tri.nor = glm::cross(tri.vert[1] - tri.vert[0], tri.vert[2] - tri.vert[1]);

                triangles.push_back(tri);
                // per-face material
                //shapes[s].mesh.material_ids[f];
            }
        }

        //link material
        utilityCore::safeGetline(fp_in, line);
        if (!line.empty() && fp_in.good()) {
            vector<string> tokens = utilityCore::tokenizeString(line);
            newGeom.materialid = atoi(tokens[1].c_str());
            cout << "Connecting Geom " << objectid << " to Material " << newGeom.materialid << "..." << endl;
        }

        //load transformations
        utilityCore::safeGetline(fp_in, line);
        while (!line.empty() && fp_in.good()) {
            vector<string> tokens = utilityCore::tokenizeString(line);

            //load tranformations
            if (strcmp(tokens[0].c_str(), "TRANS") == 0) {
                newGeom.translation = glm::vec3(atof(tokens[1].c_str()), atof(tokens[2].c_str()), atof(tokens[3].c_str()));
            }
            else if (strcmp(tokens[0].c_str(), "ROTAT") == 0) {
                newGeom.rotation = glm::vec3(atof(tokens[1].c_str()), atof(tokens[2].c_str()), atof(tokens[3].c_str()));
            }
            else if (strcmp(tokens[0].c_str(), "SCALE") == 0) {
                newGeom.scale = glm::vec3(atof(tokens[1].c_str()), atof(tokens[2].c_str()), atof(tokens[3].c_str()));
            }

            utilityCore::safeGetline(fp_in, line);
        }

        /*
        Octree mesh_tree (newGeom.bboxMin.x, newGeom.bboxMin.y, newGeom.bboxMin.z,
            newGeom.bboxMax.x, newGeom.bboxMax.y, newGeom.bboxMax.z);
        for (int i = 0; i < triangles.size(); i++) {
            Triangle& tri = triangles[i];
            tri.idx = i;
            for (int j = 0; j < 3; j++) {
                mesh_tree.insert(tri.vert[j].x, tri.vert[j].y, tri.vert[j].z);
            }
        }
        */
        // BFS to put all octree nodes into the vector

        //int index;
        //mesh_tree->find(glm::vec3(0.0f, 2.0f, 2.0f), index);
        //flatten(mesh_tree);
        //glm::vec3 ss = mesh_tree.children[0]->bottomRightBack;
        //printOctreeFlatten();
        newGeom.endTriangleIndex = triangles.size() - 1;

        newGeom.transform = utilityCore::buildTransformationMatrix(
            newGeom.translation, newGeom.rotation, newGeom.scale);
        newGeom.inverseTransform = glm::inverse(newGeom.transform);
        newGeom.invTranspose = glm::inverseTranspose(newGeom.transform);

        geoms.push_back(newGeom);
    }

}

int Scene::loadGeom(string objectid) {
    int id = atoi(objectid.c_str());
    if (id != geoms.size()) {
        cout << "ERROR: OBJECT ID does not match expected number of geoms" << endl;
        return -1;
    } else {
        cout << "Loading Geom " << id << "..." << endl;
        Geom newGeom;
        string line;

        //load object type
        utilityCore::safeGetline(fp_in, line);
        if (!line.empty() && fp_in.good()) {
            if (strcmp(line.c_str(), "sphere") == 0) {
                cout << "Creating new sphere..." << endl;
                newGeom.type = SPHERE;
            } else if (strcmp(line.c_str(), "cube") == 0) {
                cout << "Creating new cube..." << endl;
                newGeom.type = CUBE;
            }
        }

        //link material
        utilityCore::safeGetline(fp_in, line);
        if (!line.empty() && fp_in.good()) {
            vector<string> tokens = utilityCore::tokenizeString(line);
            newGeom.materialid = atoi(tokens[1].c_str());
            cout << "Connecting Geom " << objectid << " to Material " << newGeom.materialid << "..." << endl;
        }

        //load transformations
        utilityCore::safeGetline(fp_in, line);
        while (!line.empty() && fp_in.good()) {
            vector<string> tokens = utilityCore::tokenizeString(line);

            //load tranformations
            if (strcmp(tokens[0].c_str(), "TRANS") == 0) {
                newGeom.translation = glm::vec3(atof(tokens[1].c_str()), atof(tokens[2].c_str()), atof(tokens[3].c_str()));
            } else if (strcmp(tokens[0].c_str(), "ROTAT") == 0) {
                newGeom.rotation = glm::vec3(atof(tokens[1].c_str()), atof(tokens[2].c_str()), atof(tokens[3].c_str()));
            } else if (strcmp(tokens[0].c_str(), "SCALE") == 0) {
                newGeom.scale = glm::vec3(atof(tokens[1].c_str()), atof(tokens[2].c_str()), atof(tokens[3].c_str()));
            }

            utilityCore::safeGetline(fp_in, line);
        }

        newGeom.transform = utilityCore::buildTransformationMatrix(
                newGeom.translation, newGeom.rotation, newGeom.scale);
        newGeom.inverseTransform = glm::inverse(newGeom.transform);
        newGeom.invTranspose = glm::inverseTranspose(newGeom.transform);

        geoms.push_back(newGeom);
        return 1;
    }
}

int Scene::loadCamera() {
    cout << "Loading Camera ..." << endl;
    RenderState &state = this->state;
    Camera &camera = state.camera;
    float fovy;

    //load static properties
    for (int i = 0; i < 5; i++) {
        string line;
        utilityCore::safeGetline(fp_in, line);
        vector<string> tokens = utilityCore::tokenizeString(line);
        if (strcmp(tokens[0].c_str(), "RES") == 0) {
            camera.resolution.x = atoi(tokens[1].c_str());
            camera.resolution.y = atoi(tokens[2].c_str());
        } else if (strcmp(tokens[0].c_str(), "FOVY") == 0) {
            fovy = atof(tokens[1].c_str());
        } else if (strcmp(tokens[0].c_str(), "ITERATIONS") == 0) {
            state.iterations = atoi(tokens[1].c_str());
        } else if (strcmp(tokens[0].c_str(), "DEPTH") == 0) {
            state.traceDepth = atoi(tokens[1].c_str());
        } else if (strcmp(tokens[0].c_str(), "FILE") == 0) {
            state.imageName = tokens[1];
        }
    }

    string line;
    utilityCore::safeGetline(fp_in, line);
    while (!line.empty() && fp_in.good()) {
        vector<string> tokens = utilityCore::tokenizeString(line);
        if (strcmp(tokens[0].c_str(), "EYE") == 0) {
            camera.position = glm::vec3(atof(tokens[1].c_str()), atof(tokens[2].c_str()), atof(tokens[3].c_str()));
        } else if (strcmp(tokens[0].c_str(), "LOOKAT") == 0) {
            camera.lookAt = glm::vec3(atof(tokens[1].c_str()), atof(tokens[2].c_str()), atof(tokens[3].c_str()));
        } else if (strcmp(tokens[0].c_str(), "UP") == 0) {
            camera.up = glm::vec3(atof(tokens[1].c_str()), atof(tokens[2].c_str()), atof(tokens[3].c_str()));
        }

        utilityCore::safeGetline(fp_in, line);
    }

    //calculate fov based on resolution
    float yscaled = tan(fovy * (PI / 180));
    float xscaled = (yscaled * camera.resolution.x) / camera.resolution.y;
    float fovx = (atan(xscaled) * 180) / PI;
    camera.fov = glm::vec2(fovx, fovy);

	camera.right = glm::normalize(glm::cross(camera.view, camera.up));
	camera.pixelLength = glm::vec2(2 * xscaled / (float)camera.resolution.x
							, 2 * yscaled / (float)camera.resolution.y);

    camera.view = glm::normalize(camera.lookAt - camera.position);

    //set up render camera stuff
    int arraylen = camera.resolution.x * camera.resolution.y;
    state.image.resize(arraylen);
    std::fill(state.image.begin(), state.image.end(), glm::vec3());

    state.image_denoise.resize(arraylen);
    std::fill(state.image_denoise.begin(), state.image_denoise.end(), glm::vec3());

    cout << "Loaded camera!" << endl;
    return 1;
}

int Scene::loadMaterial(string materialid) {
    int id = atoi(materialid.c_str());
    if (id != materials.size()) {
        cout << "ERROR: MATERIAL ID does not match expected number of materials" << endl;
        return -1;
    } else {
        cout << "Loading Material " << id << "..." << endl;
        Material newMaterial;

        //load static properties
        for (int i = 0; i < 7; i++) {
            string line;
            utilityCore::safeGetline(fp_in, line);
            vector<string> tokens = utilityCore::tokenizeString(line);
            if (strcmp(tokens[0].c_str(), "RGB") == 0) {
                glm::vec3 color( atof(tokens[1].c_str()), atof(tokens[2].c_str()), atof(tokens[3].c_str()) );
                newMaterial.color = color;
            } else if (strcmp(tokens[0].c_str(), "SPECEX") == 0) {
                newMaterial.specular.exponent = atof(tokens[1].c_str());
            } else if (strcmp(tokens[0].c_str(), "SPECRGB") == 0) {
                glm::vec3 specColor(atof(tokens[1].c_str()), atof(tokens[2].c_str()), atof(tokens[3].c_str()));
                newMaterial.specular.color = specColor;
            } else if (strcmp(tokens[0].c_str(), "REFL") == 0) {
                newMaterial.hasReflective = atof(tokens[1].c_str());
            } else if (strcmp(tokens[0].c_str(), "REFR") == 0) {
                newMaterial.hasRefractive = atof(tokens[1].c_str());
            } else if (strcmp(tokens[0].c_str(), "REFRIOR") == 0) {
                newMaterial.indexOfRefraction = atof(tokens[1].c_str());
            } else if (strcmp(tokens[0].c_str(), "EMITTANCE") == 0) {
                newMaterial.emittance = atof(tokens[1].c_str());
            }
        }
        materials.push_back(newMaterial);
        return 1;
    }
}

/*
// Use BFS to flatten octree
void Scene::flatten(Octree * &o)
{
    int idx = 0;
    int count = 1;
    std::queue<Octree*> q;

    q.push(o);

    while (!q.empty()) {
        Octree* cur = q.front();
        q.pop();
        int triIndex = -2; // means this is a internal node
        if (cur->curTri != nullptr) {
            triIndex = cur->curTri->idx;
        }
        OctreeNode node(idx++, cur->topLeftFront, cur->bottomRightBack, triIndex);

        octrees.push_back(node);
        cout << node.idx << endl;
        if (cur->curTri != nullptr) {
            // Leaf node
            continue;
        }
        cur->children[0]->parentIndexInFlattenArr = octrees.size() - 1;
        // internal node
        for (int i = 0; i < 8; i++) {
            if (i == 0) {
                octrees[cur->children[i]->parentIndexInFlattenArr].childStartIndex = count;
            }
            count++;
            q.push(cur->children[i]);
        }
    }
}

void Scene::printOctreeFlatten()
{
    for (OctreeNode n : octrees) {
        printf("idx: %d   triangleIdx: %d  child: %d\n", n.idx, n.triangleIdx, n.childStartIndex);
        printf("min: % f %f %f\n", n.bboxMin.x, n.bboxMin.y, n.bboxMin.z);
        printf("max: % f %f %f\n", n.bboxMax.x, n.bboxMax.y, n.bboxMax.z);

        if (n.triangleIdx >= 0) {
            printf("vert1: %f %f %f\n", triangles[n.triangleIdx].vert[0].x, triangles[n.triangleIdx].vert[0].y, triangles[n.triangleIdx].vert[0].z);
            printf("vert2: %f %f %f\n", triangles[n.triangleIdx].vert[1].x, triangles[n.triangleIdx].vert[1].y, triangles[n.triangleIdx].vert[1].z);
            printf("vert3: %f %f %f\n", triangles[n.triangleIdx].vert[2].x, triangles[n.triangleIdx].vert[2].y, triangles[n.triangleIdx].vert[2].z);
        }
    }
}

*/