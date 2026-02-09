#ifndef _gosObjectPool_h_
#define _gosObjectPool_h_
#include "gosFastArray.h"


namespace gos
{
	template<class OBJ>
	class ObjectPool
	{
	public:
			ObjectPool()		{ allocator = NULL; }
			~ObjectPool()		{ unsetup(); }

		void 	setup (Allocator *allocatorIN, u32 numObjPerPage)
		{
			allocator = allocatorIN;
			num_per_page = numObjPerPage;
			pages.setup (allocator, 16);
			priv_allocPage();
		}

		void	unsetup()
		{
			if (NULL != allocator)
			{
				const u32 n = pages.getNElem();
				for (u32 i=0; i<n; i++)
				{
					GOSFREE(allocator, pages[i].blob);
				}
				pages.unsetup();
				allocator = NULL;
			}
		}

		OBJ*	alloc()
		{
			const u32 n = pages.getNElem();
			for (u32 i=0; i<n; i++)
			{
				OBJ *ret = priv_allocFromPage(i);
				if (NULL != ret)
					return ret;
			}

			priv_allocPage();
			return priv_allocFromPage(pages.getNElem()-1);
		}

		void 	free (OBJ *obj)
		{
			const u32 n = pages.getNElem();
			for (u32 i=0; i<n; i++)
			{
				Page *page = &pages[i];
				if (obj >= page->blob && obj < &page->blob[num_per_page])
				{
#ifdef _DEBUG
					const u32 index = (u32) ( ((uiPtr)obj - (uiPtr)page->blob) / sizeof(OBJ) );
					assert (index * sizeof(OBJ) + (uiPtr)page->blob == (uiPtr)obj);
#endif
					page->num_free++;
					Node *node = reinterpret_cast<Node*>( obj );
					node->next_free = page->freeList;
					page->freeList = node;
					return;
				}
			}

			DBGBREAK;
		}

	private:
		struct Node
		{
			Node	*next_free;
		};

		static_assert (sizeof(OBJ) >= sizeof(Node));

		struct Page
		{
			Node		*freeList;
			OBJ			*blob;
			u32			num_free;
		};

	private:
		OBJ*	priv_allocFromPage(u32 page_index)
		{
			assert (page_index < pages.getNElem());
			Page *page = &pages[page_index];
			if (0 == page->num_free)
				return NULL;

			page->num_free--;
			OBJ *ret = reinterpret_cast<OBJ*>( page->freeList );
			page->freeList = page->freeList->next_free;
			return ret;
		}

		void 	priv_allocPage()
		{
			Page page;
			page.num_free = num_per_page;
			page.blob = GOSALLOCT(OBJ*, allocator, sizeof(OBJ) * num_per_page);

			for (u32 i=0; i<num_per_page-1; i++)
			{
				Node *node = reinterpret_cast<Node*>(&page.blob[i]);
				node->next_free = reinterpret_cast<Node*>(&page.blob[i+1]);
			}
			Node *node = reinterpret_cast<Node*>(&page.blob[num_per_page-1]);
			node->next_free = NULL;


			page.freeList = reinterpret_cast<Node*>(page.blob);
			pages.append (page);
		}

	private:
		Allocator		*allocator;
		u32 			num_per_page;
		FastArray<Page>	pages;
	};
} //namespace gos

#endif //_gosObjectPool_h_

