#ifndef RENDERMANAGER_H
#define RENDERMANAGER_H

//#include <limits.h> must be included by platform headers!

#ifndef RS_INDEX
typedef uint32 RS_INDEX;
#endif

#define RS_INDEX_INVALID	0

class CRenderSystem;

//-----------------------------------------------------------------------------
// CRenderManager
// Purpose: Global render system manager: adds, updates and removes all rendersystems
//-----------------------------------------------------------------------------
class CRenderManager
{
	friend class CRenderSystem;
public:
	CRenderManager();
	virtual ~CRenderManager();

	RS_INDEX AddSystem(CRenderSystem *pSystem, uint32 flags = 0, int followentindex = -1, int followflags = 0, bool autoinit = true, bool enable = true);
	bool RemoveSystem(CRenderSystem *pSystem);
	bool DeleteSystem(CRenderSystem *pSystem);
	void OnSystemRemoved(const CRenderSystem *pSystem);// XDM3038c
	void DeleteAllSystems(void);
	void DeleteEntitySystems(int entindex, bool bSoftRemove);
	void DeleteSystemsBy(int entindex, const char *classname, const char *uid, bool bSoftRemove);
	void Update(const float &time, const double &elapsedTime);
	void RenderOpaque(void);
	void RenderTransparent(void);
	void CreateEntities(void);
	CRenderSystem *FindSystems(CRenderSystem *pStartSystem, bool (*pfnMatch)(CRenderSystem *pSystem, void *pData1, void *pData2, void *pData3, void *pData4), void *pData1 = NULL, void *pData2 = NULL, void *pData3 = NULL, void *pData4 = NULL);
	CRenderSystem *FindSystem(RS_INDEX index);
	CRenderSystem *FindSystemByUID(const char *uid, CRenderSystem *pStartSystem = NULL);
	CRenderSystem *FindSystemByName(const char *name, CRenderSystem *pStartSystem = NULL);
	CRenderSystem *FindSystemByFollowEntity(int entindex, CRenderSystem *pStartSystem = NULL);
	void ApplyForce(const Vector &origin, const Vector &force, float radius, bool point);
	void ListSystems(void);
	void Cmd_SearchRS(void);
	unsigned int GetFirstFreeRSUID(void);

private:
	CRenderSystem *m_pFirstSystem;// unlooped LIFO RS list. NEVER reorder it manually!
	size_t m_iAllocatedSystems;
};

extern CRenderManager *g_pRenderManager;

// FindLoadedSystems action flags
#define FINDRS_UPDATE_STATE			(1 << 0)
#define FINDRS_UPDATE_FOLLOWENT		(1 << 1)
#define FINDRS_UPDATE_ATTACHMENT	(1 << 2)
#define FINDRS_UPDATE_EVERYTHING	UI32_MAX

bool MatchRSUID(CRenderSystem *pSystem, void *pData1, void *pData2, void *pData3, void *pData4);// callback for FindSystems
uint32 FindLoadedSystems(const char *filename, int emitterindex, int followentity, int attachment, int state, uint32 actionflags, void (*pfnOnFoundSystem)(CRenderSystem *pSystem, void *pData1, void *pData2) = NULL, void *pOnFoundSystemData1 = NULL, void *pOnFoundSystemData2 = NULL);
uint32 LoadRenderSystems(const char *filename, const Vector &origin, int emitterindex, int followentity, int followmodelindex, int attachment, int fxlevel, uint32 systemflags, float timetolive, uint32 uid_postfix = 0, void (*pfnOnAddSystem)(CRenderSystem *pSystem, void *pData1, void *pData2) = NULL, void *pOnAddSystemData1 = NULL, void *pOnAddSystemData2 = NULL);

#endif // RENDERMANAGER_H
