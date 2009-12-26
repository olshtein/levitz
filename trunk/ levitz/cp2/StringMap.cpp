/*
 * StringMap.cpp
 *
 *  Created on: Dec 24, 2009
 *      Author: levio01
 */

#include "StringMap.h"
# include<iostream>
#include <string>
StringMap::StringMap() {
	_size=0;
	_root=NULL;
}

StringMap::~StringMap() {
	delete _root;
}
int StringMap::size() const{
	return _size;
}
bool StringMap::getValue (const std::string& str,int* value)const {
	if (_root==NULL) return false;
	StringMap::TREE_DEF def;
	StringMap::TreeNode * node= find(str,def,_root);
	if(def==HIMSELF){
	*value=(node->_value);
		return true;
	}
	return false;
};
bool StringMap::getValue (const char* str,int* value) {
	const std::string str2(str);
	return getValue(str2,value);
};
void StringMap::setValue(const std::string& str,int value){
	if(_root==NULL) {
		_root=new StringMap::TreeNode(str,value);
	}
	else{
		StringMap::TREE_DEF def;
		StringMap::TreeNode * node=find(str,def,_root);
		if(def==HIMSELF){
			node->_value=value;
			return;
		}
		if (def==RIGHT){
			node->_right=new StringMap::TreeNode(str,value,node);
		}
		else {
			node->_left=new StringMap::TreeNode(str,value,node);
		}
	}
	_size++;
};
void StringMap::setValue(const char* str,int value){
	setValue(std::string(str),value);
};
//
//static StringMap::TreeNode* StringMap::find(const std::string& key
//		,StringMap::TREE_DEF & def,StringMap::TreeNode* root) {
//	StringMap::TreeNode* y=root;
//	StringMap::TreeNode* x=root;
//	while (x!=NULL){
//		y=x;
//		if (x->_key>key){
//			def=LEFT;
//			x=(x->_left);
//		}
//		else if(x->_key<key){
//			x=(x->_right);
//		def=RIGHT;
//		}
//		else {
//			def=HIMSELF;
//			return y;
//		}
//	}
//	return y;
//};
const StringMap::TreeNode* StringMap::succsor(const TreeNode* a){
	if(a->_right!=NULL) return StringMap::min(a->_right);
	const StringMap::TreeNode* pernt=a->_parent;
	while(pernt!=NULL && a==pernt->_right){
		a=pernt;
		pernt=a->_parent;
	}
	return pernt;
};
const StringMap::TreeNode* StringMap::min(const TreeNode* a){
	while(a->_left!=NULL)a=a->_left;
	return a;
}

StringMapIterator StringMap::getIterator() const{
	return StringMapIterator(_root);
}
StringMapIterator::StringMapIterator(const StringMap::TreeNode *a){
	if (a!=NULL){
	_current=StringMap::min(a);
	}
	else _current=a;
}
void StringMapIterator::goNext(){
	_current= StringMap::succsor(_current);
}
int StringMapIterator::getValue() const{
	return _current->_value;
};
std::string StringMapIterator::getKey()const{
	return _current->_key;
};
bool StringMapIterator::atEnd() const{
	if(_current==NULL) return true;
	return false;
};
StringMap::TreeNode::TreeNode( const std::string& key,int value,
		TreeNode* parent):_key(key){
	_value = value;
	_left=NULL;
	_right=NULL;
	_parent=parent;
}
StringMap::TreeNode::~TreeNode(){
	if (_left!=NULL) delete _left;
	if (_right!=NULL) delete _right;
}
//bool StringMap::TreeNode::operator==( const std::string& str) const{
//	if (_key.compare(str)==0) return true;
//	return false;
//}
//bool StringMap::TreeNode::operator>( const std::string& str) const{
//	if (_key.compare(str)>0) return true;
//	return false;
//	;
//}
//bool StringMap::TreeNode::operator<(const std::string& str) const{
//	if (_key.compare(str)<0) return true;
//	return false;
//}
void StringMap::print()const{
	StringMapIterator it=getIterator();
	while(!it.atEnd()){
		std::cout<<"[key:"<<it.getKey()<<" value:"<<it.getValue()<<"] ";
		it.goNext();
	}
	std::cout<<"\n";
}


