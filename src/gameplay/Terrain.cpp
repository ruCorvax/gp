#include "Base.h"
#include "Terrain.h"
#include "TerrainPatch.h"
#include "Node.h"
#include "FileSystem.h"
#include "Camera.h"
#include <vector>
namespace gameplay
{
// Default terrain material path
#define TERRAIN_MATERIAL "res/materials/terrain.material"

// The default square size of terrain patches for a terrain that
// does not have an explicitly specified patch size.

typedef struct layer layer;
struct layer {
std::string textureMapPtr;
Vector2 textureRepeat;
std::string blendMapPtr;
int blendChannel;
};

std::vector<layer> layers;

static const unsigned int DEFAULT_TERRAIN_PATCH_SIZE = 32;

// The default height of a terrain that does not have an explicitly
// specified terrain size, expressed as a ratio of the average
// of the dimensions of the terrain heightfield:
//
//   heightMax = (image.width + image.height) / 2 * DEFAULT_TERRAIN_HEIGHT_RATIO
//
static const float DEFAULT_TERRAIN_HEIGHT_RATIO = 0.3f;

// Terrain dirty flags
static const unsigned int DIRTY_FLAG_INVERSE_WORLD = 1;

static  int _cr=-1;
static  unsigned int _patchsize=0;
Vector3 terrainSize;
Properties* _properties;
static float getDefaultHeight(unsigned int width, unsigned int height);

Terrain::Terrain() :
   _heightfield(NULL), _node(NULL), _normalMap(NULL), _flags(FRUSTUM_CULLING | LEVEL_OF_DETAIL), _dirtyFlags(DIRTY_FLAG_INVERSE_WORLD)
{
}

Terrain::~Terrain()
{
   for (size_t i = 0, count = _patches.size(); i < count; ++i)
   {
       SAFE_DELETE(_patches[i]);
   }
   SAFE_RELEASE(_normalMap);
   SAFE_RELEASE(_heightfield);
}


Terrain* Terrain::create(const char* path)
{
   return create(path, NULL);
}

Terrain* Terrain::create(Properties* properties)
{
   return create(properties->getNamespace(), properties);
}

Terrain* Terrain::create(const char* path, Properties* properties)
{
   // Terrain properties
   Properties* p = properties;
   Properties* pTerrain = NULL;
   bool externalProperties = (p != NULL);
   HeightField* heightfield = NULL;
//    Vector3 terrainSize;
   int patchSize = 0;
   int detailLevels = 1;
   float skirtScale = 0;
   const char* normalMap = NULL;
   std::string materialPath;


   if (!p && path)
   {
       p = Properties::create(path);
   }

   if (!p)
   {
       GP_WARN("Failed to properties for terrain: %s", path ? path : "");
       return NULL;
   }

   pTerrain = strlen(p->getNamespace()) > 0 ? p : p->getNextNamespace();
   if (pTerrain == NULL)
   {
       GP_WARN("Invalid terrain definition.");
       if (!externalProperties)
           SAFE_DELETE(p);
       return NULL;
   }

   // Read heightmap info
   Properties* pHeightmap = pTerrain->getNamespace("heightmap", true);
   if (pHeightmap)
   {
       // Read heightmap path
       std::string heightmap;
       if (!pHeightmap->getPath("path", &heightmap))
       {
           GP_WARN("No 'path' property supplied in heightmap section of terrain definition: %s", path);
           if (!externalProperties)
               SAFE_DELETE(p);
           return NULL;
       }

       std::string ext = FileSystem::getExtension(heightmap.c_str());
if (false)
{
  std::string path;
  std::string tiles;
path.assign( pHeightmap->getString("path"));
tiles.assign( pHeightmap->getString("tiles"));
std::stringstream ss(tiles);
std::string buf;
std::vector<std::string> files;

while (ss>>buf) files.push_back(buf);

}
       else if (ext == ".PNG")
       {
           // Read normalized height values from heightmap image
           heightfield = HeightField::createFromImage(heightmap.c_str(), 0, 1);
       }
       else if (ext == ".RAW" || ext == ".R16")
       {
           // Require additional properties to be specified for RAW files
           Vector2 imageSize;
           if (!pHeightmap->getVector2("size", &imageSize))
           {
               GP_WARN("Invalid or missing 'size' attribute in heightmap defintion of terrain definition: %s", path);
               if (!externalProperties)
                   SAFE_DELETE(p);
               return NULL;
           }

           // Read normalized height values from RAW file
           heightfield = HeightField::createFromRAW(heightmap.c_str(), (unsigned int)imageSize.x, (unsigned int)imageSize.y, 0, 1);
       }
       else
       {
           // Unsupported heightmap format
           GP_WARN("Unsupported heightmap format ('%s') in terrain definition: %s", heightmap.c_str(), path);
           if (!externalProperties)
               SAFE_DELETE(p);
           return NULL;
       }
   }
   else
   {
       // Try to read 'heightmap' as a simple string property
       std::string heightmap;
       if (!pTerrain->getPath("heightmap", &heightmap))
       {
           GP_WARN("No 'heightmap' property supplied in terrain definition: %s", path);
           if (!externalProperties)
               SAFE_DELETE(p);
           return NULL;
       }

       std::string ext = FileSystem::getExtension(heightmap.c_str());
       if (ext == ".PNG")
       {
           // Read normalized height values from heightmap image
           heightfield = HeightField::createFromImage(heightmap.c_str(), 0, 1);
       }
       else if (ext == ".RAW" || ext == ".R16")
       {
           GP_WARN("RAW heightmaps must be specified inside a heightmap block with width and height properties.");
           if (!externalProperties)
               SAFE_DELETE(p);
           return NULL;
       }
       else
       {
           GP_WARN("Unsupported 'heightmap' format ('%s') in terrain definition: %s.", heightmap.c_str(), path);
           if (!externalProperties)
               SAFE_DELETE(p);
           return NULL;
       }
   }

   // Read terrain 'size'
   if (pTerrain->exists("size"))
   {
       if (!pTerrain->getVector3("size", &terrainSize))
       {
           GP_WARN("Invalid 'size' value ('%s') in terrain definition: %s", pTerrain->getString("size"), path);
       }
   }

   // Read terrain 'patch size'
   if (pTerrain->exists("patchSize"))
   {
       patchSize = pTerrain->getInt("patchSize");
_patchsize = patchSize;
   }

   // Read terrain 'detailLevels'
   if (pTerrain->exists("detailLevels"))
   {
       detailLevels = pTerrain->getInt("detailLevels");
   }

   // Read 'skirtScale'
   if (pTerrain->exists("skirtScale"))
   {
       skirtScale = pTerrain->getFloat("skirtScale");
   }

   // Read 'normalMap'
   normalMap = pTerrain->getString("normalMap");

   // Read 'material'
   materialPath = pTerrain->getString("material", "");

   if (heightfield == NULL)
   {
       GP_WARN("Failed to read heightfield heights for terrain definition: %s", path);
       if (!externalProperties)
           SAFE_DELETE(p);
       return NULL;
   }

   if (terrainSize.isZero())
   {
       terrainSize.set(heightfield->getColumnCount(), getDefaultHeight(heightfield->getColumnCount(), heightfield->getRowCount()), heightfield->getRowCount());
   }

   if (patchSize <= 0 || patchSize > (int)heightfield->getColumnCount() || patchSize > (int)heightfield->getRowCount())
   {
       patchSize = std::min(heightfield->getRowCount(), std::min(heightfield->getColumnCount(), DEFAULT_TERRAIN_PATCH_SIZE));
   }

   if (detailLevels <= 0)
       detailLevels = 1;

   if (skirtScale < 0)
       skirtScale = 0;

   // Compute terrain scale
   Vector3 scale(terrainSize.x / (heightfield->getColumnCount()-1), terrainSize.y, terrainSize.z / (heightfield->getRowCount()-1));

   // Create terrain
   Terrain* terrain = create(heightfield, scale, (unsigned int)patchSize, (unsigned int)detailLevels, skirtScale, normalMap, materialPath.c_str(), pTerrain);

   if (!externalProperties)
       SAFE_DELETE(p);

   return terrain;
}

Terrain* Terrain::create(HeightField* heightfield, const Vector3& scale, unsigned int patchSize, unsigned int detailLevels, float skirtScale, const char* normalMapPath, const char* materialPath)
{
   return create(heightfield, scale, patchSize, detailLevels, skirtScale, normalMapPath, materialPath, NULL);
}

Terrain* Terrain::create(HeightField* heightfield, const Vector3& scale,
   unsigned int patchSize, unsigned int detailLevels, float skirtScale,
   const char* normalMapPath, const char* materialPath, Properties* properties)
{
   GP_ASSERT(heightfield);
   unsigned int width = heightfield->getColumnCount();
   unsigned int height = heightfield->getRowCount();
   // Create the terrain object
   Terrain* terrain = new Terrain();
   terrain->_heightfield = heightfield;
   terrain->_materialPath = (materialPath == NULL || strlen(materialPath) == 0) ? TERRAIN_MATERIAL : materialPath;

   // Store terrain local scaling so it can be applied to the heightfield
   terrain->_localScale.set(scale);

   // Store reference to bounding box (it is calculated and updated from TerrainPatch)
//    BoundingBox& bounds = terrain->_boundingBox;

   if (normalMapPath)
       terrain->_normalMap = Texture::Sampler::create(normalMapPath, true);


   // Read additional layer information from properties (if specified)
   if (properties)
   {
       // Parse terrain layers
       Properties* lp;
       int index = -1;

       while ((lp = properties->getNextNamespace()) != NULL)
       {
           if (strcmp(lp->getNamespace(), "layer") == 0)
           {
               // If there is no explicitly specified index for this layer, assume it's the 'next' layer
               if (lp->exists("index"))
                   index = lp->getInt("index");
               else
                   ++index;

               std::string textureMap;
               const char* textureMapPtr = NULL;
               std::string blendMap;
               const char* blendMapPtr = NULL;
               Vector2 textureRepeat;
               int blendChannel = 0;
               int row = -1, column = -1;
               Vector4 temp;

               // Read layer textures
               Properties* t = lp->getNamespace("texture", true);
               if (t)
               {
                   if (t->getPath("path", &textureMap))
                   {
                       textureMapPtr = textureMap.c_str();
                   }
                   if (!t->getVector2("repeat", &textureRepeat))
                       textureRepeat.set(1,1);
               }

               Properties* b = lp->getNamespace("blend", true);
               if (b)
               {
                   if (b->getPath("path", &blendMap))
                   {
                       blendMapPtr = blendMap.c_str();
                   }
                   const char* channel = b->getString("channel");
                   if (channel && strlen(channel) > 0)
                   {
                       char c = std::toupper(channel[0]);
                       if (c == 'R' || c == '0')
                           blendChannel = 0;
                       else if (c == 'G' || c == '1')
                           blendChannel = 1;
                       else if (c == 'B' || c == '2')
                           blendChannel = 2;
                       else if (c == 'A' || c == '3')
                           blendChannel = 3;
                   }
               }

               // Get patch row/columns that this layer applies to.
               if (lp->exists("row"))
                   row = lp->getInt("row");
               if (lp->exists("column"))
                   column = lp->getInt("column");
layer lr;
lr.textureMapPtr=textureMap;
lr.textureRepeat=textureRepeat;
lr.blendMapPtr=blendMap;
lr.blendChannel=blendChannel;
layers.push_back(lr);
               if (!terrain->setLayer(index, textureMapPtr, textureRepeat, blendMapPtr, blendChannel, row, column))
               {
                   GP_WARN("Failed to load terrain layer: %s", textureMap.c_str());
               }
           }
       }
   }


   return terrain;
}

void Terrain::setNode(Node* node)
{
   if (_node != node)
   {
       if (_node)
       {
           _node->removeListener(this);
       }

       _node = node;

       if (_node)
       {
           _node->addListener(this);
       }

       // Update patch node bindings
       for (size_t i = 0, count = _patches.size(); i < count; ++i)
       {
           _patches[i]->updateNodeBindings();
       }

       _dirtyFlags |= DIRTY_FLAG_INVERSE_WORLD;
   }
}

void Terrain::transformChanged(Transform* transform, long cookie)
{
   _dirtyFlags |= DIRTY_FLAG_INVERSE_WORLD;
}

const Matrix& Terrain::getInverseWorldMatrix() const
{
   if (_dirtyFlags & DIRTY_FLAG_INVERSE_WORLD)
   {
       _dirtyFlags &= ~DIRTY_FLAG_INVERSE_WORLD;

       if (_node)
           _inverseWorldMatrix.set(_node->getWorldMatrix());
       else
           _inverseWorldMatrix = Matrix::identity();

       // Apply local scale and invert
       _inverseWorldMatrix.scale(_localScale);
       _inverseWorldMatrix.invert();
       
   }
   return _inverseWorldMatrix;
}

bool Terrain::setLayer(int index, const char* texturePath, const Vector2& textureRepeat, const char* blendPath, int blendChannel, int row, int column)
{
   if (!texturePath)
       return false;

   // Set layer on applicable patches
   bool result = true;
   for (size_t i = 0, count = _patches.size(); i < count; ++i)
   {
       TerrainPatch* patch = _patches[i];

       if ((row == -1 || (int)patch->_row == row) && (column == -1 || (int)patch->_column == column))
       {
           if (!patch->setLayer(index, texturePath, textureRepeat, blendPath, blendChannel))
               result = false;
       }
   }

   return result;
}

Node* Terrain::getNode() const
{
   return _node;
}

bool Terrain::isFlagSet(Flags flag) const
{
   return (_flags & flag) == flag;
}

void Terrain::setFlag(Flags flag, bool on)
{
   bool changed = false;

   if (on)
   {
       if ((_flags & flag) == 0)
       {
           _flags |= flag;
           changed = true;
       }
   }
   else
   {
       if ((_flags & flag) == flag)
       {
           _flags &= ~flag;
           changed = true;
       }
   }

   if (flag == DEBUG_PATCHES && changed)
   {
       // Dirty all materials since they need to be updated to support debug drawing
       for (size_t i = 0, count = _patches.size(); i < count; ++i)
           _patches[i]->setMaterialDirty();
   }
}

unsigned int Terrain::getPatchCount() const
{
   return _patches.size();
}

TerrainPatch* Terrain::getPatch(unsigned int index) const
{
   return _patches[index];
}

const BoundingBox& Terrain::getBoundingBox() const
{
   return _boundingBox;
}

void  Terrain::update(float x,float y)
{
   
   unsigned int width = terrainSize.x;
   unsigned int height = terrainSize.z;
   unsigned int r=0,rs,rf;
   unsigned int c=0,cs,cf;
   

   float halfWidth = (width - 1) * 0.5f;
   float halfHeight = (height - 1) * 0.5f;

  r=int(std::abs((x+halfWidth)/_localScale.x)/_patchsize);
  c=int(std::abs((y+halfHeight)/_localScale.z)/_patchsize);
  if (_cr!=c+r){	
   unsigned int maxStep = (unsigned int)std::pow(2.0, (double)(3-1));
   unsigned int x1, x2, z1, z2;
   unsigned int row = 0, column = 0;
   float skirtScale=0.1;
   unsigned int h=this->_heightfield->getRowCount();
   unsigned int w=this->_heightfield->getColumnCount();
   halfWidth = (w - 1) * 0.5f;
   halfHeight = (h - 1) * 0.5f;
_cr=r+c;
   cf = c == int(h/_patchsize) ? r : r+2;
   rf = r == int(w/_patchsize) ? c : c+2;
   cs = r<2 ? 0 : r-1;
   rs = c<2 ? 0 : c-1;

   this->_patches.erase(this->_patches.begin(),this->_patches.end());

   for (unsigned int z =rs*_patchsize; z < _patchsize*rf-1; z = z2, ++row)
   {
       z1 = z;
       z2 = std::min(z1 + _patchsize, h-1);

       for (unsigned int x = cs*_patchsize; x < _patchsize*cf-1 ; x = x2, ++column)
       {

           x1 = x;
           x2 = std::min(x1 + _patchsize, w-1);

           TerrainPatch* patch = TerrainPatch::create(
this, 
this->_patches.size(), 
row, column, 
this->_heightfield->getArray(), 
w, h, 
x1, z1, x2, z2, -halfWidth, -halfHeight, 
maxStep, skirtScale );
           this->_patches.push_back(patch);

       }
   }

  for(size_t i=0,count = layers.size();i<count;++i){
this->setLayer(i, layers[i].textureMapPtr.c_str(), layers[i].textureRepeat, layers[i].blendMapPtr.c_str(), layers[i].blendChannel, -1,-1);
}
 // Read additional layer information from properties (if specified)


   for (size_t i = 0, count = this->_patches.size(); i < count; ++i)
       this->_patches[i]->updateMaterial();

  }

}

float Terrain::getHeight(float x, float z) const
{
   // Calculate the correct x, z position relative to the heightfield data.
   float cols = _heightfield->getColumnCount();
   float rows = _heightfield->getRowCount();

   GP_ASSERT(cols > 0);
   GP_ASSERT(rows > 0);

   // Since the specified coordinates are in world space, we need to use the 
   // inverse of our world matrix to transform the world x,z coords back into
   // local heightfield coordinates for indexing into the height array.
   Vector3 v = getInverseWorldMatrix() * Vector3(x, 0.0f, z);
   x = v.x + (cols - 1) * 0.5f;
   z = v.z + (rows - 1) * 0.5f;

   // Get the unscaled height value from the HeightField
   float height = _heightfield->getHeight(x, z);

   // Apply world scale to the height value
   if (_node)
   {
       Vector3 worldScale;
       _node->getWorldMatrix().getScale(&worldScale);
       height *= worldScale.y;
   }

   // Apply local scale
   height *= _localScale.y;

   return height;
}

unsigned int Terrain::draw(bool wireframe)
{
   size_t visibleCount = 0;
   for (size_t i = 0, count = _patches.size(); i < count; ++i)
   {
       visibleCount += _patches[i]->draw(wireframe);
   }
   return visibleCount;
}

static float getDefaultHeight(unsigned int width, unsigned int height)
{
   // When terrain height is not specified, we'll use a default height of ~ 0.3 of the image dimensions
   return ((width + height) * 0.5f) * DEFAULT_TERRAIN_HEIGHT_RATIO;
}

}
