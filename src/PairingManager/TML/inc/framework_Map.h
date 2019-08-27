/**************************************************************************
 * Copyright (C) 2015 Eff'Innov Technologies 
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * 
 * Developped by Eff'Innov Technologies : contact@effinnov.com
 * 
 **************************************************************************/

#ifndef MAP_H
#define MAP_H
 
 typedef enum STATUS
 {
    SUCCESS, 
    FAILED,
    NOT_FOUND,
    INVALID_MAP,
    INVALID_PARAM,
    ALREADY_EXISTS
 }STATUS;
 
 STATUS map_create(void ** map);
 
 STATUS map_destroy(void* map);
 
 STATUS map_add(void * map, void* id, void* object);
 
 STATUS map_remove(void* map, void* id);
 
 STATUS map_get(void* map, void* id, void ** object);
 
 STATUS map_getAll(void* map, void ** elements, int * lenght);

#endif /*MAP_H*/
