# Git Branching Strategy for Custom Builds

You are welcome to use whatever branching and release strategy you want for your custom builds! However we strongly recommend that you layer on top of the standard QGC git/release strategy, as this will almost certainly reduce your effort and keep quality high.

## Simplest Strategy

The simplest approach is to mirror the [QGC Git Branching process](GitBranching.md), timing your releases to fall after QGC releases. This means you release your custom builds only after upstream QGC releases it's own builds.

The next part of the document outlines how you do this. Out-of-band releases are discussed [near the end](#out_of_band).

## Daily Builds

### Development Stage

Your custom daily builds are built from master and this is where all new major feature development happens.

You must keep custom master up to date with QGC master! It is important to not lag behind, because new upstream features may require some effort to integrate with your build, or may even require changes to "core" QGC in order to work with your code. If you don't let QGC development team know soon enough, it may end up being too late to get things changed.


## Stable Builds

### Release Stage

You can do a patch release following QGC patch releases.

The process of stablization, branching and tagging should match the upstream process. Your release branch should be created from the upstream stable branch and only be synced with the upstream stable branch to bring across patches. You can apply your own patches to your code in that branch as well as needed.

### Patch Releases

Your custom patch releases should be built from your own stable branches. Make sure again to keep your stable branch in sync with upstream stable (or you will miss fixes).

Since upstream stable is always at high quality you can release your own patches based on upstream stable HEAD at any time you want (there is no need to wait on QGC to release it's next patch release).

### Out Of Band Release {#out_of_band}

An "Out of band" release is one where you want to release some of your next features prior to upstream QGC doing a patch release. In this case you create the branch for your own patch release off of your latest stable branch prior to doing any development. You then do your new feature development in that branch.

You should continue to keep this branch in sync with the associated upstream stable branch. When you are ready to release you just go through the normal tagging release process.

## Never Release Custom Stable Builds off QGC Master

You should never release any of your custom stable builds based on upstream QGC master.

If you do you will have no idea of the quality of that release since, although upstream daily build quality is fairly high, it is not as good as the upstream stable quality.
