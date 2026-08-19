#pragma once

#include "ui_framework/types.hpp"

namespace ui
{
    class Node;

    struct MousePosition
    {
        float x = 0.0f;
        float y = 0.0f;
    };

    enum class MouseButton : int
    {
        Unknown = 0,
        Left = 1,
        Middle = 2,
        Right = 3
    };

    enum class KeyCode : int
    {
        UNKNOWN = 0,

        A,
        B,
        C,
        D,
        E,
        F,
        G,
        H,
        I,
        J,
        K,
        L,
        M,
        N,
        O,
        P,
        Q,
        R,
        S,
        T,
        U,
        V,
        W,
        X,
        Y,
        Z,

       