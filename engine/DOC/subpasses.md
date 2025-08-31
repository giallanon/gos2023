Basically, you have render passes which are made up of attachments, subpasses, and subpass dependencies.

Attachments are basically just images that can be used during the rendering process, e.g. as a shader input (an "input attachment") or render target (a "color attachment).

Subpasses are groups of rendering commands, as well as a list of "references" to all of the attachments that are used throughout those rendering commands.

So, what are subpass dependencies? Well, let's say you have two subpasses:

A subpass which renders something into an attachment.

A subpass which uses that attachment as a shader input, which it uses to render something else.

In this example, the rendering being done to the first attachment in the first subpass needs to finish before it's used as a shader input in the second subpass. Otherwise, the render results will be all messed up!

Thing is, subpasses won't actually run in any specific order by default; they're allowed to run in any order for performance reasons. In some cases, that's totally fine. But, in our case, we actually need these subpasses to go in that specific order, or the render will be screwed up!

This is where subpass dependencies come in. Subpass dependencies are pretty much exactly what they sound like: dependencies between subpasses. They just simply allow us to specify the order we need to force things to happen in, in order to get the results that we want.

In our above example, we'd need one subpass dependency that goes as follows:

VkSubpassDependency dep;
dep.srcSubpass = 0;
dep.dstSubpass = 1;

dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
dep.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

dep.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
dep.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
srcSubpass is the index of the subpass we're dependant on. In this case, we need the second subpass to not start until the first subpass has finished executing, so we set srcSubpass to 0.

If we wanted to depend on a subpass that's part of a previous render pass, we could just pass in VK_SUBPASS_EXTERNAL here instead. In this case, that would mean "wait for all of the subpasses within all of the render passes before this one".

dstSubpass is the index to the current subpass, i.e. the one this dependency exists for. In this case, that's the second subpass, so dstSubpass is 1.

Now this is where it gets tricky.

srcStageMask is a bitmask of all of the Vulkan "stages" (basically, steps of the rendering process) we're asking Vulkan to finish executing within srcSubpass before we move on to dstSubpass.

In this case, we want to use the color output rendered via the first subpass in our second subpass. So, we need to wait for the first subpass to finish rendering its color outputs. Or, in other words, we need to wait until the first subpass completes its color attachment output stage. So, srcSubpass is VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT.

dstStageMask is a bitmask of all of the Vulkan stages in dstSubpass that we're not allowed to execute until after the stages in srcStageMask have completed within srcSubpass.

By passing in VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, we're telling Vulkan that it's free to execute any commands within dstSubpass in whatever order it wants to, except for commands related to the fragment shader. We're purposely not allowing Vulkan to execute those commands until after the srcStageMask stages have finished executing within srcSubpass.

Okay, so, now we've specified the execution dependencies (the order in which these subpasses must execute) between our two subpasses. But GPUs are complicated beasts that do a lot of caching of images and such, so just specifying the order we need these rendering commands to occur in actually isn't enough. We also need to tell Vulkan the memory access types we need and when we need them, so it can update caches and such accordingly.

So, srcAccessMask is a bitmask of all the Vulkan memory access types used by srcSubpass, and dstAccessMask is a bitmask of all the Vulkan memory access types we're going to use in dstSubpass.

Think of it like we're saying: "after you've finished writing to the color attachment in srcSubpass, 'flush' the results as needed for us to be able to read it in our shaders."

All of these values can be messed with as needed to make Vulkan synchronize subpasses/render passes basically however the heck you want it to. It's all very complicated, but also very powerful. Just like most of Vulkan really.

Hope this all helped explain stuff better!

If you want more info, in addition to the other pages people have already been linking, I'd recommend watching 
this excellent talk on render passes (https://www.youtube.com/watch?v=x2SGVjlVGhE)
and reading 
chapter 8 in the Vulkan spec.