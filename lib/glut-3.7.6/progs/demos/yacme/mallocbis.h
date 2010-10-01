
#define newItem(item,list,type)\
	if (list == NULL)\
		item = (type *) malloc(sizeof(type));\
	else\
	{\
		item = list;\
		list = list->next;\
	}

#define freeItem(item,list)\
	item->next = list;\
	list = item;
