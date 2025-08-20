# ​ Computer Graphics Lab Works

A collection of Computer Graphics lab assignments and personal projects, implemented in **C++**.

---

## Table of Contents

-   [Lab 1](#-lab-1)
-   [Lab 2](#-lab-2)
-   [My-Ideas](#my-ideas)

---

## Lab 1

-   **`midpoint_circle_with_dotted_border`**  
    Implements the **Midpoint Circle Algorithm** (also known as Bresenham’s Circle Algorithm) with a **dotted border effect**, demonstrating circle drawing using symmetry and selective plotting.

-   **`multiple_midpoint_circles_rainbow_colors`**  
    Draws multiple concentric circles with alternating rainbow colors. It:
    -   Uses the **Midpoint Circle Algorithm** iteratively.
    -   Takes the number of circles as input.
    -   Maintains a consistent radius difference between each circle.
    -   Demonstrates iterative drawing and color transitions.

---

## Lab 2

-   **`bold_line_using_8_neighbour`**  
    Implements a line drawing algorithm where each pixel of the line is thickened using the 8-neighbour technique.

    -   Produces a bold line instead of a thin rasterized line.
    -   Demonstrates pixel manipulation for enhanced line width.

-   **`line_with_circles_at_endpoints`**  
    Draws a line between two points, and at each endpoint a circle is drawn with its center at the line’s endpoint.
    -   Combines line drawing with the Midpoint Circle Algorithm.
    -   Useful for visualizing connected nodes or graphs with circular endpoints.

---

## My-Ideas

-   **`national_martyrs_monument`**  
    A custom implementation of the **National Martyrs’ Monument of Bangladesh** using **C++ and OpenGL**.

    -   The monument’s triangular structure is built with **line drawing algorithms**, with optional bold rendering using the 8-neighbour pixel plotting technique.
    -   Geometry is calculated dynamically:
        -   **Base points** are divided into equal segments for symmetry.
        -   **Slanting lines** are drawn from the base to the top apex.
        -   **Bars and triangles** in the middle portion are derived from distance calculations and line intersections.
    -   Uses the **Euclidean distance formula** to compute segment lengths and relative positions:

        d = √((x₂ - x₁)² + (y₂ - y₁)²)

        This formula is applied to determine line segment lengths, distances from endpoints, and placement of connecting bars.

    -   Demonstrates **procedural generation**: the monument’s entire structure emerges from mathematical computation (midpoints, distances, segment gaps) rather than static coordinates.
    -   The diagram is **interactive**: it maintains its proportions relative to the frame size. You can resize the frame however you like, and the diagram will still stay fully visible and scale accordingly.
    -   Highlights how combining **basic line algorithms** with geometry can recreate a complex national landmark.

---
