#ifndef _PARISC_DMA_MAPPING_H
#define _PARISC_DMA_MAPPING_H

#include <linux/mm.h>
#include <linux/config.h>
#include <asm/cacheflush.h>

/*
** See Documentation/DMA-mapping.txt
*/
struct hppa_dma_ops {
	int  (*dma_supported)(struct device *dev, u64 mask);
	void *(*alloc_consistent)(struct device *dev, size_t size, dma_addr_t *iova, int flag);
	void *(*alloc_noncoherent)(struct device *dev, size_t size, dma_addr_t *iova, int flag);
	void (*free_consistent)(struct device *dev, size_t size, void *vaddr, dma_addr_t iova);
	dma_addr_t (*map_single)(struct device *dev, void *addr, size_t size, enum dma_data_direction direction);
	void (*unmap_single)(struct device *dev, dma_addr_t iova, size_t size, enum dma_data_direction direction);
	int  (*map_sg)(struct device *dev, struct scatterlist *sg, int nents, enum dma_data_direction direction);
	void (*unmap_sg)(struct device *dev, struct scatterlist *sg, int nhwents, enum dma_data_direction direction);
	void (*dma_sync_single)(struct device *dev, dma_addr_t iova, unsigned long offset, size_t size, enum dma_data_direction direction);
	void (*dma_sync_sg)(struct device *dev, struct scatterlist *sg, int nelems, enum dma_data_direction direction);
};

/*
** We could live without the hppa_dma_ops indirection if we didn't want
** to support 4 different coherent dma models with one binary (they will
** someday be loadable modules):
**     I/O MMU        consistent method           dma_sync behavior
**  =============   ======================       =======================
**  a) PA-7x00LC    uncachable host memory          flush/purge
**  b) U2/Uturn      cachable host memory              NOP
**  c) Ike/Astro     cachable host memory              NOP
**  d) EPIC/SAGA     memory on EPIC/SAGA         flush/reset DMA channel
**
** PA-7[13]00LC processors have a GSC bus interface and no I/O MMU.
**
** Systems (eg PCX-T workstations) that don't fall into the above
** categories will need to modify the needed drivers to perform
** flush/purge and allocate "regular" cacheable pages for everything.
*/

#ifdef CONFIG_PA11
extern struct hppa_dma_ops pcxl_dma_ops;
extern struct hppa_dma_ops pcx_dma_ops;
#endif

extern struct hppa_dma_ops *hppa_dma_ops;

static inline void *
dma_alloc_coherent(struct device *dev, size_t size, dma_addr_t *dma_handle,
		   int flag)
{
	return hppa_dma_ops->alloc_consistent(dev, size, dma_handle, flag);
}

static inline void *
dma_alloc_noncoherent(struct device *dev, size_t size, dma_addr_t *dma_handle,
		      int flag)
{
	return hppa_dma_ops->alloc_noncoherent(dev, size, dma_handle, flag);
}

static inline void
dma_free_coherent(struct device *dev, size_t size, 
		    void *vaddr, dma_addr_t dma_handle)
{
	hppa_dma_ops->free_consistent(dev, size, vaddr, dma_handle);
}

static inline void
dma_free_noncoherent(struct device *dev, size_t size, 
		    void *vaddr, dma_addr_t dma_handle)
{
	hppa_dma_ops->free_consistent(dev, size, vaddr, dma_handle);
}

static inline dma_addr_t
dma_map_single(struct device *dev, void *ptr, size_t size,
	       enum dma_data_direction direction)
{
	return hppa_dma_ops->map_single(dev, ptr, size, direction);
}

static inline void
dma_unmap_single(struct device *dev, dma_addr_t dma_addr, size_t size,
		 enum dma_data_direction direction)
{
	hppa_dma_ops->unmap_single(dev, dma_addr, size, direction);
}

static inline int
dma_map_sg(struct device *dev, struct scatterlist *sg, int nents,
	   enum dma_data_direction direction)
{
	return hppa_dma_ops->map_sg(dev, sg, nents, direction);
}

static inline void
dma_unmap_sg(struct device *dev, struct scatterlist *sg, int nhwentries,
	     enum dma_data_direction direction)
{
	hppa_dma_ops->unmap_sg(dev, sg, nhwentries, direction);
}

static inline dma_addr_t
dma_map_page(struct device *dev, struct page *page, unsigned long offset,
	     size_t size, enum dma_data_direction direction)
{
	return dma_map_single(dev, (page_address(page) + (offset)), size, direction);
}

static inline void
dma_unmap_page(struct device *dev, dma_addr_t dma_address, size_t size,
	       enum dma_data_direction direction)
{
	dma_unmap_single(dev, dma_address, size, direction);
}


static inline void
dma_sync_single(struct device *dev, dma_addr_t dma_handle, size_t size,
		enum dma_data_direction direction)
{
	if(hppa_dma_ops->dma_sync_single)
		hppa_dma_ops->dma_sync_single(dev, dma_handle, 0, size, direction);
}

static inline void
dma_sync_single_range(struct device *dev, dma_addr_t dma_handle,
		      unsigned long offset, size_t size,
		      enum dma_data_direction direction)
{
	if(hppa_dma_ops->dma_sync_single)
		hppa_dma_ops->dma_sync_single(dev, dma_handle, offset, size, direction);
}

static inline void
dma_sync_sg(struct device *dev, struct scatterlist *sg, int nelems,
		 enum dma_data_direction direction)
{
	if(hppa_dma_ops->dma_sync_sg)
		hppa_dma_ops->dma_sync_sg(dev, sg, nelems, direction);
}

static inline int
dma_supported(struct device *dev, u64 mask)
{
	return hppa_dma_ops->dma_supported(dev, mask);
}

static inline int
dma_set_mask(struct device *dev, u64 mask)
{
	if(!dev->dma_mask || !dma_supported(dev, mask))
		return -EIO;

	*dev->dma_mask = mask;

	return 0;
}

static inline int
dma_get_cache_alignment(void)
{
	return dcache_stride;
}

static inline int
dma_is_consistent(dma_addr_t dma_addr)
{
	return (hppa_dma_ops->dma_sync_single == NULL);
}

static inline void
dma_cache_sync(void *vaddr, size_t size,
	       enum dma_data_direction direction)
{
	if(hppa_dma_ops->dma_sync_single)
		flush_kernel_dcache_range((unsigned long)vaddr, size);
}

static inline void *
parisc_walk_tree(struct device *dev)
{
	struct device *otherdev;
	if(likely(dev->platform_data != NULL))
		return dev->platform_data;
	/* OK, just traverse the bus to find it */
	for(otherdev = dev->parent; otherdev;
	    otherdev = otherdev->parent) {
		if(otherdev->platform_data) {
			dev->platform_data = otherdev->platform_data;
			break;
		}
	}
	BUG_ON(!dev->platform_data);
	return dev->platform_data;
}
		
#define GET_IOC(dev) (HBA_DATA(parisc_walk_tree(dev))->iommu);	
	

#ifdef CONFIG_IOMMU_CCIO
struct parisc_device;
struct ioc;
void * ccio_get_iommu(const struct parisc_device *dev);
int ccio_request_resource(const struct parisc_device *dev,
		struct resource *res);
int ccio_allocate_resource(const struct parisc_device *dev,
		struct resource *res, unsigned long size,
		unsigned long min, unsigned long max, unsigned long align,
		void (*alignf)(void *, struct resource *, unsigned long, unsigned long),
		void *alignf_data);
#else /* !CONFIG_IOMMU_CCIO */
#define ccio_get_iommu(dev) NULL
#define ccio_request_resource(dev, res) request_resource(&iomem_resource, res)
#define ccio_allocate_resource(dev, res, size, min, max, align, alignf, data) \
		allocate_resource(&iomem_resource, res, size, min, max, \
				align, alignf, data)
#endif /* !CONFIG_IOMMU_CCIO */

#ifdef CONFIG_IOMMU_SBA
struct parisc_device;
void * sba_get_iommu(struct parisc_device *dev);
#endif

#endif
