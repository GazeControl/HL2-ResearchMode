using UnityEngine;

public class IndicatorDisplay : MonoBehaviour
{
    public TextMesh DisplayText;

    public void SetText(string label)
    {
        DisplayText.text = label;
    }

    public void SetPosition(Vector3 pos)
    {
        gameObject.transform.position = pos;
    }
}