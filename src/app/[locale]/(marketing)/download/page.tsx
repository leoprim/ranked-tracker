import type { Metadata } from 'next';
import { getTranslations, setRequestLocale } from 'next-intl/server';

type IDownloadProps = {
  params: Promise<{ locale: string }>;
};

export async function generateMetadata(props: IDownloadProps): Promise<Metadata> {
  const { locale } = await props.params;
  const t = await getTranslations({
    locale,
    namespace: 'Download',
  });

  return {
    title: t('meta_title'),
    description: t('meta_description'),
  };
}

export default async function Download(props: IDownloadProps) {
  const { locale } = await props.params;
  setRequestLocale(locale);
  const t = await getTranslations({
    locale,
    namespace: 'Download',
  });

  return (
    <>
      <h2 className="mt-5 text-2xl font-bold">{t('title')}</h2>
      <p className="text-base">{t('description')}</p>

      <div className="mt-6">
        <a
          href="/api/download"
          className="inline-block rounded-lg bg-blue-700 px-6 py-3 text-lg font-semibold text-white no-underline hover:bg-blue-800"
        >
          {t('download_button')}
        </a>
        <p className="mt-2 text-sm text-gray-500">{t('platform_note')}</p>
      </div>

      <h3 className="mt-8 text-xl font-bold">{t('requirements_title')}</h3>
      <ul className="mt-3 list-disc pl-6 text-base">
        <li>{t('req_os')}</li>
        <li>{t('req_obs')}</li>
      </ul>

      <h3 className="mt-8 text-xl font-bold">{t('install_title')}</h3>
      <ol className="mt-3 list-decimal pl-6 text-base">
        <li>{t('step_1')}</li>
        <li>{t('step_2')}</li>
        <li>{t('step_3')}</li>
        <li>{t('step_4')}</li>
      </ol>

      <h3 className="mt-8 text-xl font-bold">{t('setup_title')}</h3>
      <ol className="mt-3 list-decimal pl-6 text-base">
        <li>{t('setup_1')}</li>
        <li>{t('setup_2')}</li>
        <li>{t('setup_3')}</li>
        <li>{t('setup_4')}</li>
      </ol>

      <div className="mt-8 rounded-lg border border-gray-200 bg-gray-50 p-4">
        <p className="text-sm font-semibold">{t('api_title')}</p>
        <p className="mt-1 text-sm text-gray-600">{t('api_description')}</p>
      </div>
    </>
  );
}
