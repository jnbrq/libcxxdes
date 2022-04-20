# TODO List

## Basic cache hierarchy

Question: Why a cache hierarchy?

1. Not much different than using explicit scratchpads and globally-understood mechanism. For simulation purposes, they work fine. One can simply extend to explicitly-managed mechanisms. We leave the question of workload-dependent caches later.
2. Easy to implement structure. Plays well with double-buffered scratchpads. (do we really need double buffered scratchpads?)
3. Composable. Cache hierarchy naturally fit in the hierarchical design that we talk about.

## A simple systolic array model

Take a look at ScaleSim for the details of such a model.
