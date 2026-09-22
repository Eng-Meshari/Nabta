<!-- markdownlint-disable MD012 MD032 -->

# Nabta Project Instructions

## Project structure

- ESP32-CAM firmware is located in `firmware/Nabta`.
- PlatformIO configuration is located in the root `platformio.ini`.
- Python server is located in `server`.
- The trained model is `best.pt`.
- Hardware wiring documentation is in `docs/WIRING.md`.

## Firmware

- Board: AI Thinker ESP32-CAM.
- Framework: Arduino through PlatformIO.
- Camera: OV2640 with external PSRAM.
- DHT11 data pin: GPIO 13.
- Passive buzzer signal pin: GPIO 2.
- Serial baud rate: 115200.
- The project uses Wi-Fi, camera capture, DHT11 readings, buzzer alerts, and HTTP communication.

## Safety rules

- Do not change GPIO assignments unless explicitly requested.
- Do not modify `secrets.h` or expose Wi-Fi credentials.
- Do not delete files or rewrite the whole project unnecessarily.
- Preserve the existing project structure.
- Make the smallest safe change needed.
- Do not change hardware behavior without explaining the effect.

## Required workflow

Before editing:
1. Inspect the relevant files.
2. Explain the planned changes briefly.
3. Identify possible risks.

After editing:
1. Run PlatformIO Build for the ESP32-CAM firmware.
2. Run Python syntax checks for changed server files.
3. Report all modified files.
4. Report any remaining errors.
5. Do not claim success unless verification was completed.



<!--

دليل كتابة الطلبات إلى Codex
هذا القسم مرجع للمستخدم فقط، ولا يُعد مهمة مطلوب تنفيذها تلقائيًا.

==================================================

1. طلب تعديل أو إضافة ميزة جديدة

الوظيفة:
يُستخدم هذا القالب عندما تريد إضافة ميزة، تعديل وظيفة، أو إصلاح جزء معين.
يجب توضيح المطلوب، سبب التعديل، القيود، والنتيجة المطلوبة.

النص المقترح:

Task:
[اكتب المطلوب بدقة.]

Context:
[اشرح سبب التعديل وكيف يفترض أن يعمل.]

Relevant files:
[اكتب أسماء الملفات المتوقعة، أو اطلب من Codex البحث عنها.]

Constraints:
- Preserve the existing project structure.
- Do not change GPIO assignments unless explicitly requested.
- Do not modify or expose secrets.h.
- Make the smallest safe change.
- Do not remove existing functionality.

Acceptance criteria:
- [اكتب النتيجة التي تعتبر بها المهمة ناجحة.]
- PlatformIO Build must succeed.
- Run all relevant checks.

==================================================

2. طلب فحص المشروع قبل التعديل

الوظيفة:
يُرسل هذا الأمر قبل أي تعديل كبير.
يجعل Codex يقرأ الملفات، يفهم المشكلة، ويقترح خطة دون تعديل الكود.
استخدمه عندما لا تعرف الملفات التي تحتاج إلى تعديل أو تريد مراجعة الخطة أولًا.

الأمر:

Inspect the project and propose a plan. Do not edit files yet.

المعنى:
افحص المشروع واقترح خطة، لكن لا تعدل أي ملف حتى أوافق.

==================================================

3. الموافقة على خطة Codex وبدء التنفيذ

الوظيفة:
يُرسل هذا الأمر بعد أن يعرض Codex خطته وتوافق عليها.
يسمح له بتنفيذ الخطة بأقل تعديلات ممكنة، ثم تشغيل Build والفحوصات.

الأمر:

Implement the approved plan, make minimal changes, and run all relevant checks.

المعنى:
نفّذ الخطة التي وافقت عليها، وأجرِ أقل تعديلات ممكنة، ثم شغّل جميع الفحوصات المناسبة.

==================================================

4. إرسال خطأ ظهر في PlatformIO أو المشروع

الوظيفة:
يُستخدم عند ظهور خطأ في Build أو Upload أو Serial Monitor أو السيرفر.
يجب لصق رسالة الخطأ كاملة، وليس آخر سطر فقط.

النص المقترح:

An error occurred while building or running the project.

Error type:
[PlatformIO Build / Upload / Serial Monitor / Python server / Other]

Full error message:
[الصق رسالة الخطأ كاملة هنا.]

Please:
1. Inspect the complete error.
2. Identify the exact cause.
3. Identify the affected file and line when possible.
4. Propose the smallest safe fix.
5. Do not edit files until you explain the cause and plan.
6. Do not modify secrets.h.
7. Do not change GPIO assignments.

المعنى:
حدث خطأ أثناء بناء المشروع أو تشغيله.
افحص الخطأ كاملًا، حدد السبب والملف والسطر، ثم اقترح أقل إصلاح ممكن دون تعديل الملفات قبل شرح الخطة.

==================================================

5. السماح لـCodex بإصلاح الخطأ

الوظيفة:
يُرسل بعد أن يشرح Codex سبب الخطأ ويقترح الإصلاح وتوافق عليه.
يطلب منه تطبيق الإصلاح ثم إعادة الاختبار.

الأمر:

Apply the proposed fix with minimal changes. Then run PlatformIO Build or the relevant checks again and report the result.

المعنى:
طبّق الإصلاح المقترح بأقل تعديلات ممكنة، ثم شغّل PlatformIO Build أو الفحوصات المناسبة مرة أخرى وأرسل النتيجة.

==================================================

6. طلب مراجعة التغييرات قبل اعتمادها

الوظيفة:
يُستخدم للتأكد من أن التعديلات صحيحة ولم تؤثر في وظائف أخرى.

الأمر:

Review the changes for bugs, regressions, hardware conflicts, and security issues. Do not make additional changes until you report your findings.

المعنى:
راجع التعديلات وابحث عن أخطاء أو تأثيرات جانبية أو تعارضات في الهاردوير أو مشكلات أمنية، ولا تنفذ تعديلات إضافية قبل عرض النتائج.

==================================================

7. طلب ملخص نهائي

الوظيفة:
يُرسل بعد انتهاء المهمة لمعرفة الملفات التي تغيرت، سبب التغيير، ونتائج الاختبارات.

الأمر:

Summarize:
1. What was changed.
2. Which files were modified.
3. Why each change was necessary.
4. Which checks were run.
5. Whether PlatformIO Build succeeded.
6. Any remaining risks or manual hardware tests required.

المعنى:
اعرض ملخصًا للتعديلات، الملفات المتأثرة، سبب كل تعديل، الفحوصات التي شُغلت، نتيجة Build، وأي اختبارات هاردوير ما زالت مطلوبة.

==================================================

8. طلب فحص فقط دون تغيير الملفات

الوظيفة:
يُستخدم عندما تريد معرفة سبب مشكلة أو فهم جزء من المشروع فقط.

الأمر:

Diagnose the issue and explain the cause. Do not modify any files.

المعنى:
شخّص المشكلة واشرح سببها فقط، دون تعديل أي ملف.

==================================================

9. طلب معرفة طريقة عمل جزء من المشروع

الوظيفة:
يُستخدم لفهم الكود أو تتبع تدفق البيانات دون إجراء تعديل.

الأمر:

Explain how this feature works, including the execution flow, relevant files, functions, inputs, outputs, and dependencies. Do not modify any files.

المعنى:
اشرح طريقة عمل الميزة، وتسلسل التنفيذ، والملفات والدوال والمدخلات والمخرجات والمكتبات المرتبطة بها، دون تعديل الملفات.

==================================================

10. القاعدة الأفضل للمهام الكبيرة

ابدأ دائمًا بهذا الأمر:

Inspect the project and propose a plan. Do not edit files yet.

بعد مراجعة الخطة والموافقة عليها، أرسل:

Implement the approved plan, make minimal changes, and run all relevant checks.

بعد انتهاء التنفيذ، أرسل:

Review the changes for bugs, regressions, hardware conflicts, and security issues. Do not make additional changes until you report your findings.

وأخيرًا اطلب الملخص النهائي باستخدام الأمر الموجود في القسم رقم 7.

-->