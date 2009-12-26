/*
 * StringMap.h
 *
 *  Created on: Dec 24, 2009
 *      Author: levio01
 */

#ifndef STRINGMAP_H_
#define STRINGMAP_H_
#include <iostream>
#include <string>
#include <stdlib.h>
#include <utility>
class StringMapIterator;
class StringMap {
	friend class StringMapIterator;
public:
	enum TREE_DEF{
		RIGHT,LEFT,HIMSELF
	};
	StringMap();
	~StringMap();
	int size() const;
	/*If the string is in the map, sets the int pointer to the associated value
		and returns true. Otherwise, returns false.*/
	bool getValue (const std::string& str,int* value) const;
	//receives a char pointer and an int pointer. Works like above.
	bool getValue (const char* str,int* value);
	// Associates the int with the string. If the string already existed,
	//updates the value associated with the string.
	void setValue(const std::string& str,int value);
	//a char pointer and an int. Works like above.
	void setValue(const char* str,int value);
	//returns a StringMapIterator object. The iterator visits each node in the tree
	//in ascending alphabetical order. For usage details,
	//see the demonstration code above.
	StringMapIterator getIterator() const;
	void print()const;
private:
	class TreeNode{
	public:
		int _value;
		std::string _key;
		TreeNode* _left;
		TreeNode* _right;
		TreeNode* _parent;
//		TreeNode(const TreeNode& a){
//			_key=a._key;
//			_left=a._left;
//			_right=a._right;
//			_parent=a._parent;
//		}
		TreeNode(const std::string& key,int value,TreeNode* parent=NULL);
		~TreeNode();
		//		bool operator==( const std::string& ) const;
		//		bool operator>( const std::string& ) const;
		//		bool operator<( const std::string& ) const;
		//	private:
		//		TreeNode(const TreeNode& a){};
		//		TreeNode& operator=(const TreeNode& a);
	};

	TreeNode* _root;
	int _size;
	static const TreeNode* succsor(const TreeNode* a);
	static const TreeNode* min(const TreeNode* a);
	/* if this key in the StringMap return a pointer to it else
	 * return pointer to the parent TreeNode where this key should be.
	 * set the def to repesnt if accordinly LEFT,RIGHT,HIMSELF */
	static TreeNode* find(const std::string& key,TREE_DEF& def,TreeNode* root){
		StringMap::TreeNode* y=root;
		StringMap::TreeNode* x=root;
		while (x!=NULL){
			y=x;
			if (x->_key>key){
				def=LEFT;
				x=(x->_left);
			}
			else if(x->_key<key){
				x=(x->_right);
				def=RIGHT;
			}
			else {
				def=HIMSELF;
				return y;
			}
		}
		return y;
		;
	}
	//	static TreeNode* find(const std::string& key,TREE_DEF& def,TreeNode* root){return NULL;};
};
class StringMapIterator {
public:
	StringMapIterator(StringMap::TreeNode const *a =NULL);
	void goNext();
	int getValue() const;
	std::string getKey()const;
	bool atEnd() const;
private:
	StringMap::TreeNode const *_current;

};
#endif /* STRINGMAP_H_ */
