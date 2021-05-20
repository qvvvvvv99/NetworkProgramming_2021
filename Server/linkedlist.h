#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include "define.h"



//접속자 노드 추가
ClntList* Add_ClntList();

//리스트에 노드 추가
void Add_List(ClntList *Node);

//접속자 노드 제거
void Delete_ClntNode(ClntList *Node);

//모든 접속자 노드 제거
void Delete_AllNode();

//접속자 노드 정렬
void sort_list();

#endif
