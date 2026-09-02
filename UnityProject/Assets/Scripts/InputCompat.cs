using UnityEngine;
#if ENABLE_INPUT_SYSTEM
using UnityEngine.InputSystem;
using UnityEngine.InputSystem.UI;
#endif
using UnityEngine.EventSystems;

namespace Project0.Unity
{
    /// <summary>
    /// Input compatibility helper bridging legacy Unity Input Manager and the new Input System package (com.unity.inputsystem).
    /// </summary>
    public static class InputCompat
    {
        public static bool GetKey(KeyCode key)
        {
#if ENABLE_INPUT_SYSTEM
            var kbd = Keyboard.current;
            if (kbd != null)
            {
                switch (key)
                {
                    case KeyCode.W:
                    case KeyCode.UpArrow:
                        return kbd.wKey.isPressed || kbd.upArrowKey.isPressed;
                    case KeyCode.S:
                    case KeyCode.DownArrow:
                        return kbd.sKey.isPressed || kbd.downArrowKey.isPressed;
                    case KeyCode.A:
                    case KeyCode.LeftArrow:
                        return kbd.aKey.isPressed || kbd.leftArrowKey.isPressed;
                    case KeyCode.D:
                    case KeyCode.RightArrow:
                        return kbd.dKey.isPressed || kbd.rightArrowKey.isPressed;
                    case KeyCode.Space:
                        return kbd.spaceKey.isPressed;
                    case KeyCode.Return:
                        return kbd.enterKey.isPressed || kbd.numpadEnterKey.isPressed;
                    case KeyCode.Escape:
                        return kbd.escapeKey.isPressed;
                    case KeyCode.R:
                        return kbd.rKey.isPressed;
                    case KeyCode.C:
                        return kbd.cKey.isPressed;
                    case KeyCode.LeftShift:
                    case KeyCode.RightShift:
                        return kbd.leftShiftKey.isPressed || kbd.rightShiftKey.isPressed;
                }
            }
#endif
#if ENABLE_LEGACY_INPUT_MANAGER
            return Input.GetKey(key);
#else
            return false;
#endif
        }

        public static bool GetKeyDown(KeyCode key)
        {
#if ENABLE_INPUT_SYSTEM
            var kbd = Keyboard.current;
            if (kbd != null)
            {
                switch (key)
                {
                    case KeyCode.W:
                    case KeyCode.UpArrow:
                        return kbd.wKey.wasPressedThisFrame || kbd.upArrowKey.wasPressedThisFrame;
                    case KeyCode.S:
                    case KeyCode.DownArrow:
                        return kbd.sKey.wasPressedThisFrame || kbd.downArrowKey.wasPressedThisFrame;
                    case KeyCode.A:
                    case KeyCode.LeftArrow:
                        return kbd.aKey.wasPressedThisFrame || kbd.leftArrowKey.wasPressedThisFrame;
                    case KeyCode.D:
                    case KeyCode.RightArrow:
                        return kbd.dKey.wasPressedThisFrame || kbd.rightArrowKey.wasPressedThisFrame;
                    case KeyCode.Space:
                        return kbd.spaceKey.wasPressedThisFrame;
                    case KeyCode.Return:
                        return kbd.enterKey.wasPressedThisFrame || kbd.numpadEnterKey.wasPressedThisFrame;
                    case KeyCode.Escape:
                        return kbd.escapeKey.wasPressedThisFrame;
                    case KeyCode.R:
                        return kbd.rKey.wasPressedThisFrame;
                    case KeyCode.C:
                        return kbd.cKey.wasPressedThisFrame;
                    case KeyCode.LeftShift:
                    case KeyCode.RightShift:
                        return kbd.leftShiftKey.wasPressedThisFrame || kbd.rightShiftKey.wasPressedThisFrame;
                }
            }
#endif
#if ENABLE_LEGACY_INPUT_MANAGER
            return Input.GetKeyDown(key);
#else
            return false;
#endif
        }

        public static void EnsureUIInputModule(GameObject eventSystemObj)
        {
            if (eventSystemObj == null) return;

#if ENABLE_INPUT_SYSTEM
            if (eventSystemObj.GetComponent<InputSystemUIInputModule>() == null)
            {
                eventSystemObj.AddComponent<InputSystemUIInputModule>();
            }
#endif
#if ENABLE_LEGACY_INPUT_MANAGER
            if (eventSystemObj.GetComponent<StandaloneInputModule>() == null &&
                eventSystemObj.GetComponent("InputSystemUIInputModule") == null)
            {
                eventSystemObj.AddComponent<StandaloneInputModule>();
            }
#endif
        }
    }
}
