

/**
 * @file TensorTypes.cpp
 * @brief Implementation of 2D tensor operations for RogueCity core data structures.
 *
 * Contains methods for adding and rotating 2D tensors.
 */

namespace RogueCity::Core {

    /**
     * @brief Adds another Tensor2D to this tensor.
     *
     * Performs element-wise addition of the tensor components.
     * The 'smooth' parameter is unused in the core layer, but may be used in generator layers for smooth blending.
     *
     * @param other The Tensor2D to add.
     * @param smooth Indicates whether smooth blending should be applied (unused here).
     * @return Reference to this Tensor2D after addition.
     */
    Tensor2D& Tensor2D::add(const Tensor2D& other, [[maybe_unused]] bool smooth);

    /**
     * @brief Rotates the tensor by a given angle in radians.
     *
     * Applies a rotation transformation to the tensor components using double-angle trigonometric formulas.
     *
     * @param theta_radians The angle to rotate the tensor, in radians.
     * @return Reference to this Tensor2D after rotation.
     */
    Tensor2D& Tensor2D::rotate(double theta_radians);

} // namespace RogueCity::Core
