2025-09-21
    -   cachare i (GPUDescrSetLayoutHandle) descriptor-set ed eventualmente riutilizzarli visto che sono dei descrittori, non e' 
        necessario crearne N diversi che descrivono la stessa cosa

    -   memorizzare il layout attuale delle VkImage (RT, Texture, SwapchainImg..) per evitare di fare "mageTransition()" quando non e'
        strettamente necessario

    -   RT, ZB, Texture... alla fine sono tutti delle VkImage, non e' possibile unificare le interfaccie e il tipo?