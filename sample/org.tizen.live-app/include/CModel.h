/*
 * Copyright 2013  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://floralicense.org
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

class CModel {
public:
	/*! \TODO: Fill me */
	static CModel *GetInstance();

	void *GetTaggedObject(const char *tag);
	int AddTaggedObject(const char *tag, void *object);
	void *DelTaggedObject(const char *tag);

private:
	CModel(void);
	virtual ~CModel(void);

	static CModel *m_pInstance;
	Eina_List *m_pList;
};

/* End of a file */
