package jp.ac.tsukuba.cs.softlab.translator;

import org.eclipse.core.resources.IProject;
import org.eclipse.core.resources.IProjectDescription;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.ui.IWorkbenchPropertyPage;
import org.eclipse.ui.dialogs.PropertyPage;

public class MessagePackPropertyPage extends PropertyPage implements IWorkbenchPropertyPage {
    private Button check;
    /** ユーザー・インターフェイスを作成 */
    protected Control createContents(Composite parent) {
        Composite composite = new Composite(parent, SWT.NULL);
        composite.setLayout(new GridLayout());
        composite.setLayoutData(
            new GridData(GridData.FILL_BOTH));
        check = new Button(composite, SWT.CHECK);
        check.setText("IDLをclassに変換する");
        try {
            // 初期値をセット
            IProject project = (IProject)getElement();
            check.setSelection(project.hasNature(
                                        ClassNature.NATURE_ID));
        } catch(CoreException ex){
            ex.printStackTrace();
        }
        return composite;
    }
    /** OKボタンが押下された場合の処理 */
    public boolean performOk() {
        try {
            if(check.getSelection()){
                addNature();
            } else {
                removeNature();
            }
            return true;
        } catch(CoreException ex){
            ex.printStackTrace();
            return false;
        }
    }
    /** ネイチャを追加する */
    private void addNature() throws CoreException {
        IProject project = (IProject)getElement();
        IProjectDescription description =
            project.getDescription();
        String[] natures = description.getNatureIds();
        for (int i = 0; i < natures.length; ++i) {
            if (ClassNature.NATURE_ID.equals(natures[i])) {
                return;
            }
        }
        String[] newNatures = new String[natures.length + 1];
        System.arraycopy(natures, 0, newNatures
                                         , 0, natures.length);
        newNatures[natures.length] = ClassNature.NATURE_ID;
        description.setNatureIds(newNatures);
        project.setDescription(description, null);
    }
    /** ネイチャを削除する */
    private void removeNature() throws CoreException {
        IProject project = (IProject)getElement();
        IProjectDescription description =
            project.getDescription();
        String[] natures = description.getNatureIds();
        for (int i = 0; i < natures.length; ++i) {
            if (ClassNature.NATURE_ID.equals(natures[i])) {
                String[] newNatures =
                    new String[natures.length - 1];
                System.arraycopy(natures, 0, newNatures, 0, i);
                System.arraycopy(natures, i + 1, newNatures
                                 , i, natures.length - i - 1);
                description.setNatureIds(newNatures);
                project.setDescription(description, null);
                return;
            }
        }
    }
}