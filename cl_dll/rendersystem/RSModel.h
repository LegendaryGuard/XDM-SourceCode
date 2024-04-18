#ifndef RSMODEL_H
#define RSMODEL_H

//-----------------------------------------------------------------------------
// A very convinient way to draw a studio model
//-----------------------------------------------------------------------------
class CRSModel : public CRenderSystem
{
	typedef CRenderSystem BaseClass;
public:
	virtual const char *GetClassName(void) const { return "CRSModel"; }// XDM3038b
	CRSModel(void);
	CRSModel(const Vector &origin, const Vector &angles, const Vector &velocity, int entindex, struct model_s *pModel, int body, int skin, int sequence, int r_mode, int r_fx, byte r, byte g, byte b, float a, float adelta, float scale, float scaledelta, float framerate, float timetolive);
	virtual ~CRSModel(void);

	//virtual void KillSystem(void);
	void FreeData(void);// XDM3038c
	//virtual void ResetAllParameters(void);
	void ResetParameters(void);
	virtual bool ParseKeyValue(const char *szKey, const char *szValue);// XDM3038b
	virtual void InitializeSystem(void);// XDM3038b
	virtual bool Update(const float &time, const double &elapsedTime);
	virtual void UpdateFrame(const float &time, const double &elapsedTime);
	virtual void Render(void);
	virtual void CreateEntities(void);

	bool InitModel(model_t *pModel);
	struct cl_entity_s *GetEntity(void) { return m_pEntity; }

	int m_iBSPLeaf;
	int m_iModelBody;
	int m_iModelTextureGroup;
	int m_iModelSequence;
	struct model_s *m_pModel;

private:
	struct cl_entity_s *m_pEntity;
	//studiohdr_t *m_pModelData;
	//int m_iModelIndex;
};

#endif // RSMODEL_H
